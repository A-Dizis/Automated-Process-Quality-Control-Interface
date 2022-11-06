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
    double *current = NULL, c_temp;
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
        check_pointer = realloc(current, length * sizeof(double));
        if (check_pointer == NULL)
        {
            printf("couldn't reallocate voltage memory block...\n");
            return -1;
        }
        current = check_pointer;

        voltage[length - 1] = -v_temp;
        current[length - 1] = c_temp;
    }
   
    // declarations after length has been set
    double rsquared_right[length];
    int max_rsquared_right = 0;
    double current_1_der[length];
    double *current_1_der_ptr = &current_1_der[0];
    double current_2_der[length];
    double *current_2_der_ptr = &current_2_der[0];
    double c0, c1, c_cov00, c_cov01, c_cov11, c_sumsq;
    double d0, d1, d_cov00, d_cov01, d_cov11, d_sumsq;
    double Vd;
    double current_at_600V = current[59];

    // First derivative of current
    differentation(&current, &voltage, &current_1_der_ptr, length);    
    differentation(&current_1_der_ptr, &voltage, &current_2_der_ptr, length);  


    // r squared right calculations
    {
        for (int i = 0; i < length; i++)
        {
            rsquared_right[i] = 0;
        }
        for (int i = 20; i < length; i++)
        {
            rsquared_right[length-1-i] = gsl_stats_correlation(&voltage[length-1-i], 1, &current[length-1-i], 1, i);
        }
        for(int i = 0; i < length; i++)
        {
            rsquared_right[length-1-i] = rsquared_right[length-1-i] * rsquared_right[length-1-i];
        }
    }


    // calculate maximal rsquared for fitting in accumulation
    max_rsquared_right = (int)gsl_stats_max_index(rsquared_right, 1, length);

    // linear fit in left
    d0 = 0;
    gsl_fit_mul(voltage, 1, current, 1, max_rsquared_right, /*&d0,*/ &d1, /*&d_cov00, &d_cov01,*/ &d_cov11, &d_sumsq);
    fprintf(gnuPipe, "h(x) = %e * x\n", d1);

    // linear fit in right
    c0 = gsl_stats_mean(&current[60], 1, length-60);
    c1 = 0;
    fprintf(gnuPipe, "g(x) = %e \n", c0);
    

    // output read data to console for check
    for (int i = 0; i < length; i++)
    {
        printf("|%4d:  %.3lf V   %.3E F^-2|\n", i, voltage[i], current[i]);
        fprintf(fp2, "%.3E    %.3E    %.3E    %.3E    %.3E    %.3E\n", voltage[i], current[i], rsquared_right[i], current_2_der[i], current_2_der[i]);
    }

    
    // Vd from diagram cross point of linear fits
    Vd = 10e11*((c0-d0)/(10e11*(d1-c1)));
    //fprintf(gnuPipe, "set arrow from %e, graph 0 to %e, graph 1 nohead\n", Vd, Vd);


    printf("c0:%E    c1:%E\nd0:%E    d1:%E\n", c0, c1, d0, d1);
    printf("max_rsquared_right:%lf\n", rsquared_right[max_rsquared_right]);
    printf("max_rsquared r:%d\n", max_rsquared_right);
    printf("Vd:%lf(V)    V[600V]:%.3E(A)\n", Vd);


    {
        char buf[2048] = {};
        int n = 0;
        n = n + sprintf(&buf[n],"set terminal png size 800,800\n");
        n = n + sprintf(&buf[n],"set output '%s.png'\n", argv[1]);
        n = n + sprintf(&buf[n],"set xlabel \"Voltage(V)\" \nshow xlabel\n");
        n = n + sprintf(&buf[n],"set ylabel \"Current(A)\"\nshow xlabel\n"); 
        //n = n + sprintf(&buf[n],"set label 'y=Vd' at %e+1, graph 0.5 font \"Source Code Pro Semibold\"\n", Vd);
        n = n + sprintf(&buf[n],"set label \" Vd:%.3lf(V)  \\n I[600V]: %.3E(A)\" at graph 0.1, graph 0.8 font \"Source Code Pro Semibold\"\n",  Vd, current_at_600V);
        n = n + sprintf(&buf[n],"plot '%s.dat' u 1:2 with lines linestyle 10 linewidth 3 linecolor 'black' \n", argv[1]);
        //n = n + sprintf(&buf[n],"'' u 1:5 with lines linestyle 10 linewidth 1.5 linecolor 'grey30' axis x1y2 \n", argv[1]);
        //n = n + sprintf(&buf[n],"[%e:%e] g(x) linestyle 0 linewidth 2 linecolor 'red' title 'g(x)=%e',", voltage[max_rsquared_right], voltage[length-1], c0);
        //n = n + sprintf(&buf[n],"[%e:%e] h(x) linestyle 0 linewidth 2 linecolor 'blue' title 'h(x)=%ex'\n", voltage[0], voltage[max_rsquared_left],  d1);                   
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
