#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_spline.h>
#include "AnalysisFunctions.h"

// pass filename as argument
// later we will use batch to
// send all files in the folder
// for consecutive execution
int main(int argc, char *argv[])
{
    double *voltage = NULL, v_temp;
    double *inv_capacitance_2 = NULL, c_temp;
    double *check_pointer;


    // data length
    int length = 0;


    // file pipes
    FILE *fp = NULL, *fp2 = NULL;
    FILE *gnuPipe = NULL;



    // reading file to get data
    if (argc < 2)
    {
        printf("no filename given. exiting...\n");
        return -1;
    }


    // open file
    read_file(&fp, &fp2, &argv[1]);


    // create plot using the pipe
    gnuPipe = popen("gnuplot -presistent", "w");

    
    // ignore first 5 rows as it's no data
    ignore_lines(&fp, 5);


    // read the data from the file
    while (fscanf(fp, "%lf\t%lf\n", &v_temp, &c_temp) == 2)
    {
        length = length + 1;
        check_pointer = realloc(voltage, length * sizeof(double));
        if (check_pointer == NULL)
        {
            printf("couldn't reallocate voltage memory block...\n");
            return -1;
        }
        voltage = check_pointer;
        check_pointer = realloc(inv_capacitance_2, length * sizeof(double));
        if (check_pointer == NULL)
        {
            printf("couldn't reallocate voltage memory block...\n");
            return -1;
        }
        inv_capacitance_2 = check_pointer;

        voltage[length - 1] = -v_temp;
        inv_capacitance_2[length - 1] = 1.0/(c_temp*c_temp);
    }
    /*
    for(int i=0; i<length; length++)
    {
        c_temp = inv_capacitance_2[i];
        inv_capacitance_2[i] = inv_capacitance_2[length-1-i];
        inv_capacitance_2[length-1-i] = c_temp;

        v_temp = voltage[i];
        voltage[i] = voltage[length-1-i];
        voltage[length-1-i] = c_temp;
    }
*/

    // declarations after length has been set
    double rsquared_right[length];
    int max_rsquared_right = 0;
    double rsquared_left[length];
    int max_rsquared_left = 0;
    double c0, c1, c_cov00, c_cov01, c_cov11, c_sumsq;
    double d0, d1, d_cov00, d_cov01, d_cov11, d_sumsq;
    double Vd;

    // r squared left calculations
    {
        for (int i = 0; i < length; i++)
        {
            rsquared_left[i] = 0;
        }
        for (int i = 25; i < length; i++)
        {
            rsquared_left[i] = gsl_stats_correlation(&voltage[5], 1, &inv_capacitance_2[5], 1, i-6);
            rsquared_left[i] = rsquared_left[i] * rsquared_left[i];
        }
    }


    // r squared right calculations
    {
        for (int i = 0; i < length; i++)
        {
            rsquared_right[i] = 0;
        }
        for (int i = 28; i < length; i++)
        {
            rsquared_right[length-1-i] = gsl_stats_correlation(&voltage[length-1-i], 1, &inv_capacitance_2[length-1-i], 1, i);
        }
        for(int i = 0; i < length; i++)
        {
            rsquared_right[length-1-i] = rsquared_right[length-1-i] * rsquared_right[length-1-i];
        }
    }


    // calculate maximal rsquared for fitting in accumulation
    max_rsquared_right = (int)gsl_stats_max_index(rsquared_right, 1, length);
    max_rsquared_left = (int)gsl_stats_max_index(rsquared_left, 1, length);

    // linear fit in left
    d0 = 0;
    gsl_fit_linear(voltage, 1, inv_capacitance_2, 1, max_rsquared_left, &d0, &d1, &d_cov00, &d_cov01, &d_cov11, &d_sumsq);
    fprintf(gnuPipe, "h(x) = %e * x + %e\n", d1, d0);

    // linear fit in right
    c0 = gsl_stats_mean(&inv_capacitance_2[60], 1, length-60);
    c1 = 0;
    fprintf(gnuPipe, "g(x) = %e \n", c0);
    

    // output read data to console for check
    for (int i = 0; i < length; i++)
    {
        printf("|%4d:  %.3lf V   %.3E F^-2|\n", i, voltage[i], inv_capacitance_2[i]);
        fprintf(fp2, "%.3E    %.3E    %.3E    %.3E\n", voltage[i], inv_capacitance_2[i], rsquared_left[i], rsquared_right[i]);
    }

    
    // Vd from diagram cross point of linear fits
    Vd = 10e11*((c0-d0)/(10e11*(d1-c1)));
    fprintf(gnuPipe, "set arrow from %e, graph 0 to %e, graph 1 nohead\n", Vd, Vd);


    printf("c0:%E    c1:%E\nd0:%E    d1:%E\n", c0, c1, d0, d1);
    printf("max_rsquared_left:%lf    max_rsquared_right:%lf\n", rsquared_left[max_rsquared_left], rsquared_right[max_rsquared_right]);
    printf("max_rsquared l:%d    max_rsquared r:%d\n", max_rsquared_left, max_rsquared_right);
    printf("Vd:%lf(V) \n", Vd);


    {
        char buf[2048] = {};
        int n = 0;
        n = n + sprintf(&buf[n],"set terminal png size 800,800\n");
        n = n + sprintf(&buf[n],"set output '%s.png'\n", argv[1]);
        n = n + sprintf(&buf[n],"set xlabel \"Voltage(V)\" \nshow xlabel\n");
        n = n + sprintf(&buf[n],"set ylabel \"1/Capacitance^2(1/F^2)\"\nshow xlabel\n"); 
        n = n + sprintf(&buf[n],"set label 'y=Vd' at %e+1, graph 0.5 font \"Source Code Pro Semibold\"\n", Vd);
        n = n + sprintf(&buf[n],"set label \" Vd:%.3lf(V)\" at graph 0.1, graph 0.8 font \"Source Code Pro Semibold\"\n",  Vd);
        n = n + sprintf(&buf[n],"plot '%s.dat' u 1:2 with lines linestyle 10 linewidth 1.5 linecolor 'grey30',", argv[1]);
        n = n + sprintf(&buf[n],"[%e:%e] g(x) linestyle 0 linewidth 3 linecolor 'red' title 'g(x)=%e',", voltage[max_rsquared_right], voltage[length-1], c0);
        n = n + sprintf(&buf[n],"[%e:%e] h(x) linestyle 0 linewidth 3 linecolor 'blue' title 'h(x)=%ex+%e'\n", voltage[5], voltage[max_rsquared_left],  d1, d0);                   
        //n = n + sprintf(&buf[n],"[:] [%e:%e] q(y) linestyle 0 linewidth 2 linecolor 'blue' title 'y=Vd'\n", 0.0, gsl_stats_max(current, 1, length));

        printf("Gnuplot buffer: %d characters\n", n);
        fprintf(gnuPipe, "%s\n", buf);
    }


    // close file desciptors
    fclose(fp);
    fclose(fp2);
    pclose(gnuPipe);
    // clean temporary data file
    remove_dat_file(&argv[1]);

    return 0;
}
