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
    double *current = NULL, c_temp;
    double *voltage = NULL, v_temp;
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
    while (fscanf(fp, "%lf\t%lf\n", &c_temp, &v_temp) == 2)
    {
        length = length + 1;
        check_pointer = realloc(current, length * sizeof(double));
        if (check_pointer == NULL)
        {
            printf("couldn't reallocate voltage memory block...\n");
            return -1;
        }
        current = check_pointer;
        check_pointer = realloc(voltage, length * sizeof(double));
        if (check_pointer == NULL)
        {
            printf("couldn't reallocate voltage memory block...\n");
            return -1;
        }
        voltage = check_pointer;

        current[length - 1] = c_temp;
        voltage[length - 1] = v_temp;
    }

    // declarations after length has been set
    double c0, c1, c_cov00, c_cov01, c_cov11, c_sumsq;
    double Cmedian;


    // linear fit in right
    gsl_fit_linear(&current[2], 1, &voltage[2], 1, length-2, &c0, &c1, &c_cov00, &c_cov01, &c_cov11, &c_sumsq);
   
    fprintf(gnuPipe, "g(x) =%E*x + %E\n", c1, c0);
    

    // output read data to console for check
    for (int i = 0; i < length; i++)
    {
        printf("|%4d:  %.3EA   %.3E V|\n", i, current[i], voltage[i]);
        fprintf(fp2, "%.3E    %.3E\n", current[i], voltage[i]);
    }


    printf("c0:%E V\n", c0);

    int n = 0;
    {
        char buf[2048] = "";
        n = n + sprintf(&buf[n],"set terminal pngcairo size 800,800\n");
        n = n + sprintf(&buf[n],"set output '%s.png'\n", argv[1]);     
        n = n + sprintf(&buf[n],"set label \"Rsh:%lf omega/square \\nDeltaRsh:%e\" at graph 0.7, graph 0.3 \n", M_PI * c1 /(log(2)), sqrt(M_PI * fabs(c_cov11) /(log(2)))); 
        n = n + sprintf(&buf[n],"set xlabel \"Voltage(V)\" \nshow xlabel\n");
        n = n + sprintf(&buf[n],"set ylabel \"Current(A)\"\nshow xlabel\n");   
        n = n + sprintf(&buf[n],"plot '%s.dat' u 1:2 with points pointtype 7 pointsize 1,", argv[1]);
        n = n + sprintf(&buf[n],"[%e:%e] g(x) linestyle 2 linewidth 2 linecolor 'blue' title 'g(x)=%E + x%E V'\n", current[0], current[length-1], c0, c1);
 
        printf("Rsh:%lf    DeltaRsh:%e\n", M_PI * c1 /(log(2)), sqrt(M_PI * fabs(c_cov11) /(log(2))));
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