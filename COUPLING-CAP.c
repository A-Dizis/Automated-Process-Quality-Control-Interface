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
    double *capacitance = NULL, c_temp;
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
        check_pointer = realloc(capacitance, length * sizeof(double));
        if (check_pointer == NULL)
        {
            printf("couldn't reallocate voltage memory block...\n");
            return -1;
        }
        capacitance = check_pointer;

        voltage[length - 1] = -v_temp;
        capacitance[length - 1] = c_temp;
    }

    // declarations after length has been set
    double sortedCapacitance[length];   
    double c0,c1;
    double Cmedian;


    //sorting a temp capacitance from smaller to bigger for median
    for(int i=0; i<length; i++)
    {
        sortedCapacitance[i] = capacitance[i];
    }

    for(int i=0; i<length; i++)
    {
        double max = sortedCapacitance[0];
        int max_l = 0;
        for(int j=0; j<length-i; j++)
        {
            if(max<sortedCapacitance[j])
            {
                max = sortedCapacitance[j];
                max_l = j;
            }
        }
        sortedCapacitance[max_l] = sortedCapacitance[length-1-i];
        sortedCapacitance[length-1-i] = max;    
    }



    // linear fit in right
    //    gsl_fit_linear(&voltage[max_rsquared_right], 1, &inv_capacitance_2[max_rsquared_right], 1, length-1-max_rsquared_right, &c0, &c1, &c_cov00, &c_cov01, &c_cov11, &c_sumsq);
    c0 = gsl_stats_median(sortedCapacitance, 1, length);
    c1 = 0;
    fprintf(gnuPipe, "g(x) = %e \n", c0);
    

    // output read data to console for check
    for (int i = 0; i < length; i++)
    {
        printf("|%4d:  %.3lf V   %.3E F^-2|\n", i, voltage[i], capacitance[i]);
        fprintf(fp2, "%.3E    %.3E\n", voltage[i], capacitance[i]);
    }


    printf("c0:%E V\n", c0);

    int n = 0;
    {
        char buf[2048] = "";
        n = n + sprintf(&buf[n],"set terminal pngcairo size 800,800\n");
        n = n + sprintf(&buf[n],"set output '%s.png'\n", argv[1]);        
        n = n + sprintf(&buf[n],"plot '%s.dat' u 1:2 with points,", argv[1]);
        n = n + sprintf(&buf[n],"[%e:%e] g(x) linestyle 0 linewidth 2 linecolor 'red' title 'median=%e V'\n", voltage[0], voltage[length-1], c0);
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