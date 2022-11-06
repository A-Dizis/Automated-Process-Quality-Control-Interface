#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_fit.h>
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
    FILE *fp = NULL, *fp2 = NULL, *fp3 = NULL;
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
    double log_current[length], log_current_1_der[length],log_current_2_der[length];
    double* log_current_ptr = &log_current[0];
    double* log_current_1_der_ptr = &log_current_1_der[0];
    double* log_current_2_der_ptr = &log_current_2_der[0];
    double current_1_der[length], current_2_der[length];
    double* current_1_der_ptr = &current_1_der[0];
    double* current_2_der_ptr = &current_2_der[0];
    double d0, d1, d_cov00, d_cov01, d_cov11, d_sumsq;
    double c;
    int max_2_dev_spline, min_2_dev_spline;
    double Vth1, Vth2, Vth3;


    // logarithm of the current and 1st and 2nd derivatives
    for(int i = 0; i < length; i++)
    {
        log_current[i] = log(current[i]);
    }    
    //differentation(&log_current_ptr, &voltage, &log_current_1_der_ptr, length);
    //differentation(&log_current_1_der_ptr, &voltage, &log_current_2_der_ptr, length);


    // centered differences first derivative
    // scale y-axis appropriately here 10e8
    differentation(&current, &voltage, &current_1_der_ptr, length);


    // calculate saddle points of first derivative
    size_t min_1_dev=5, max_1_dev=5;
    gsl_stats_minmax_index(&min_1_dev, &max_1_dev, current_1_der_ptr, 1, length);


    // centered differences second derivative
    // scale y-axis appropriately here 10e8
    differentation(&current_1_der_ptr, &voltage, &current_2_der_ptr, length);


    // calculate saddle points of second derivative
    size_t min_2_dev=5, max_2_dev=5;
    gsl_stats_minmax_index(&min_2_dev, &max_2_dev, current_2_der_ptr, 1, length);


    // linear fit in slope
    gsl_fit_linear(&voltage[max_2_dev], 1, &current[max_2_dev], 1, min_2_dev-max_2_dev, &d0, &d1, &d_cov00, &d_cov01, &d_cov11, &d_sumsq);
    fprintf(gnuPipe, "h(x) = %e + %e * x\n", d0, d1);

    // extrapolation to max g_m
    c = current[max_1_dev]-d1*voltage[max_1_dev];
    Vth1 = -c/d1;


    // second derivative  method with interpolation
    fp3 = fopen("interp.dat","w");
    gsl_interp_accel *acc = gsl_interp_accel_alloc ();
    gsl_spline *spline = gsl_spline_alloc (gsl_interp_cspline, min_2_dev-max_2_dev+11);
    gsl_spline_init(spline, &voltage[max_2_dev-5], &current_2_der[max_2_dev-5], min_2_dev-max_2_dev + 11);
    double spline_x[500], spline_y[500];
    for (int i=0; i<500; i++)
    {
        double q = voltage[max_2_dev-5] + (voltage[min_2_dev+5]-voltage[max_2_dev-5])*(i+0.5)/500.0;
        spline_y[i] = gsl_spline_eval(spline, q, acc);
        spline_x[i] = q;
        //fprintf(fp3, "%lf    %lf\n", spline_x[i], spline_y[i]);
    }
    gsl_spline_free (spline);
    gsl_interp_accel_free (acc);
    max_2_dev_spline = gsl_stats_max_index(spline_y, 1, 500);
    Vth2 = spline_x[max_2_dev_spline];




/*
    // second derivative of logarithm method with interpolation
    double spline1_x[length], spline1_y[length];
    double *spline1_x_ptr = spline1_x, *spline1_y_ptr = spline1_y;
    
    acc = gsl_interp_accel_alloc ();
    spline = gsl_spline_alloc (gsl_interp_steffen, length);
    gsl_spline_init(spline, voltage, log_current, length);
    for (int i=0; i<length; i++)
    {
        double q = voltage[0] + (voltage[length]-voltage[0])*(i)/(double)length;
        spline1_y[i] = gsl_spline_eval(spline, q, acc);
        spline1_x[i] = q;
        fprintf(fp3, "%lf    %lf\n", spline1_x[i], spline1_y[i]);
    }
    differentation(&spline1_y_ptr, &voltage, &log_current_1_der_ptr, length);
    differentation(&log_current_1_der_ptr, &spline1_x_ptr, &log_current_2_der_ptr, length);
    gsl_spline_free (spline);
    gsl_interp_accel_free (acc);
*/
    for(int i = 0; i < length; i++)
    {
        log_current_2_der[i]=current_2_der[i]/current[i]-current_1_der[i]*current_1_der[i]/(current[i]*current[i]);
    }
    min_2_dev_spline = gsl_stats_min_index(&log_current_2_der[40], 1, length-40);
    Vth3 = spline_x[min_2_dev_spline];




    // output read data to console for check
    for (int i = 0; i < length; i++)
    {
        printf("|%4d:  %.3lf V   %.3E A|\n", i, voltage[i], current[i]);
        fprintf(fp2, "%.3E    %.3E    %.3E    %.3E    %.3E\n", voltage[i], current[i], current_1_der[i], current_2_der[i], log_current_2_der[i]);
    }


    printf("d0:%E    d1:%E\n", d0, d1);
    printf("Vth1:%lf    Vth2:%lf\n", Vth1, Vth2);
    printf("Mean Vth:%lf\n", (Vth1+Vth2)/2.0);
    printf("Second derivative method:%lf", Vth3);


    {
        char buf[2048] = {};
        int n = 0;
        n = n + sprintf(&buf[n],"set terminal png size 800,800\n");
        n = n + sprintf(&buf[n],"set output '%s.png'\n", argv[1]);
        n = n + sprintf(&buf[n],"set xlabel \"Voltage(V)\" \nshow xlabel\n");
        n = n + sprintf(&buf[n],"set ylabel \"Current(A)\"\nshow xlabel\n"); 
        n = n + sprintf(&buf[n],"set label \" Vth(method1):%.3lf(V) \\n Vth(method2):%.3lf(V) \\n Vth(mean):%.3lf(V) \\n Inclination:%.3E \" at graph 0.1, graph 0.8 font \"Source Code Pro Semibold\"\n",  Vth1, Vth2, (Vth1+Vth2)/2.0, d1);
        n = n + sprintf(&buf[n],"set y2tics\n");
        n = n + sprintf(&buf[n],"set x2tics\n");
        n = n + sprintf(&buf[n],"plot '%s.dat' u 1:2 with lines linestyle 10 linewidth 1.5 linecolor 'grey30',", argv[1]);
        //n = n + sprintf(&buf[n],"'' u 1:3 with lines linestyle 10 linewidth 1.5 linecolor 'green' axes x1y2,");
        n = n + sprintf(&buf[n],"[%e:%e] h(x) linestyle 0 linewidth 3 linecolor 'blue' title 'h(x)=%ex+%e'\n", voltage[min_2_dev], voltage[max_2_dev], d1, c);                   

        printf("Gnuplot buffer: %d characters\n", n);
        fprintf(gnuPipe, "%s\n", buf);
    }


    // close file desciptors
    fclose(fp);
    fclose(fp2);
    fclose(fp3);
    pclose(gnuPipe);
    // clean temporary data file
    remove_dat_file(&argv[1]);

    return 0;
}