#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sort.h>
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
        length++;
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

        voltage[length - 1] = v_temp;
        current[length - 1] = c_temp;
    }


    // declarations after length has been set
    double rsquared[length];
    double rightside[50];
    int max_rsquared = 0, min_rsquared = 0;
    double c0, d0;


    // r squared calculations accumulation
    {
        for (int i = 0; i < length; i++)
        {
            rsquared[i] = 0;
        }
        for (int i = 0; i < 40; i++)
        {
            rsquared[length/2+i] = gsl_stats_correlation(&voltage[length/2-10], 1, &current[length/2-10], 1, i+9);
        }
        for (int i = 0; i < length; i++)
        {
            rsquared[i] = rsquared[i] * rsquared[i];
        }
    }

    max_rsquared = (int)gsl_stats_max_index(rsquared, 1, length);

    {
        for (int i = 0; i < length; i++)
        {
            rsquared[i] = 0;
        }
        for (int i = 70; i < max_rsquared; i++)
        {
            rsquared[i] = gsl_stats_correlation(&voltage[i], 1, &current[i], 1, max_rsquared-50);
        }
        for (int i = 0; i < length; i++)
        {
            rsquared[i] = rsquared[i] * rsquared[i];
        }
    }

    min_rsquared = (int)gsl_stats_max_index(rsquared, 1, length);

    // Average in accumulation
    c0 = gsl_stats_mean(&current[min_rsquared], 1, max_rsquared-min_rsquared);
    fprintf(gnuPipe, "g(x) = %e \n", c0);

    // superior averaging algorithm on sorted data
    for(int i=0; i<50; i++)
    {
        rightside[i]=current[length-i];
    }
    gsl_sort(rightside, 1, 50);

    d0 = gsl_stats_trmean_from_sorted_data(0.15, rightside, 1, 50);
    fprintf(gnuPipe, "h(x) = %e \n", d0);



    // output read data to console for check
    for (int i = 0; i < length; i++)
    {
        printf("|%4d:  %.3lf V   %.3E A    %lf|\n", i, voltage[i], current[i], rsquared[i]);
        fprintf(fp2, "%.3E    %.3E\n", voltage[i], current[i]);
    }

    printf("c0:%E    d0:%E\n", c0, d0);
    printf("voltage[max_rsquared]:%lf(V)    voltage[lenght]:%lf(V)\n",voltage[max_rsquared], voltage[length-1]);
    printf("max_rsquared:%d    min_rsquared:%d\n", max_rsquared, min_rsquared);
    
    {
        char buf[2048] = "";
        int n = 0;
        n = n + sprintf(&buf[n],"set terminal pngcairo size 800 ,800\n");
        n = n + sprintf(&buf[n],"set output '%s.png'\n", argv[1]);
        n = n + sprintf(&buf[n],"set label \" Isurf:%.3E(A) \" at graph 0.1, graph 0.8 font \"Source Code Pro Semibold\"\n",  c0-d0);
        n = n + sprintf(&buf[n],"set xlabel \"Voltage(V)\" \nshow xlabel\n");
        n = n + sprintf(&buf[n],"set ylabel \"Current(A)\"\nshow xlabel\n"); 
        n = n + sprintf(&buf[n],"plot '%s.dat' u 1:2 with lines linestyle 10 linewidth 1.5 linecolor 'grey30',", argv[1]);
        n = n + sprintf(&buf[n],"[%e:%e] g(x) linestyle 1 linewidth 3 linecolor 'red' title 'g(x)=%e',", voltage[min_rsquared], voltage[max_rsquared], c0);
        n = n + sprintf(&buf[n],"[%e:%e] h(x) linestyle 1 linewidth 3 linecolor 'blue' title 'h(x)=%e'\n", voltage[150], voltage[length-1], d0);                   

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