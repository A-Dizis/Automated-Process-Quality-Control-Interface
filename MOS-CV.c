#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_math.h>
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
        length++;
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

        voltage[length - 1] = v_temp;
        capacitance[length - 1] = c_temp;
    }


    // declarations after length has been set
    double capacitance_1_der[length];
    double* capacitance_1_der_ptr = &capacitance_1_der[0];
    double capacitance_2_der[length];
    double* capacitance_2_der_ptr = &capacitance_2_der[0];
    double inv_cap_2[length];
    double inv_cap_2_max;
    double rsquared[length];
    int max_rsquared = 0;
    double c0, c1, c_cov00, c_cov01, c_cov11, c_sumsq;
    double d0, d1, d_cov00, d_cov01, d_cov11, d_sumsq;
    double f0, f1, f_cov00, f_cov01, f_cov11, f_sumsq;
    double Vfb, Cox, Tox, Nox;
    double phi_b, phi_ms, Nbulk;
    
    // calculate inverse square of capacitance
    for(int i = 0; i <length; i++)
    {
        inv_cap_2[i] = 1/pow(capacitance[i], 2);
    }


    // centered differences first derivative
    // scale y-axis appropriately here 10e8
    differentation(&capacitance, &voltage, &capacitance_1_der_ptr, length);


    // calculate saddle points of first derivative
    size_t min_1_dev=5, max_1_dev=5;
    gsl_stats_minmax_index(&min_1_dev, &max_1_dev, capacitance_1_der_ptr, 1, length);


    // centered differences second derivative
    // scale y-axis appropriately here 10e8
    differentation(&capacitance_1_der_ptr, &voltage, &capacitance_2_der_ptr, length);


    // calculate saddle points of second derivative
    size_t min_2_dev=5, max_2_dev=5;
    gsl_stats_minmax_index(&min_2_dev, &max_2_dev, capacitance_2_der_ptr, 1, length);


    // r squared calculations
    {
        double temp_voltage[length],temp_current[length];
        for (int i = 0; i < length; i++)
        {
            temp_voltage[i] = voltage[length-1-i];
            temp_current[i] = capacitance[length-1-i];
        }
        for (int i = 0; i < 10; i++)
        {
            rsquared[length-1-i] = 0;
        }
        for (int i = 10; i < length-max_1_dev; i++)
        {
            rsquared[length-1-i] = gsl_stats_correlation(temp_voltage, 1, temp_current, 1, i);
            rsquared[i] = rsquared[i] * rsquared[i];
        }
         for (int i = length-max_1_dev ; i < length; i++)
        {
            rsquared[length-1-i] = 0;
        }
    }


    // calculate maximal rsquared for fitting in accumulation
    max_rsquared = (int)gsl_stats_max_index(rsquared, 1, length);


    // linear fit in accumulation
    gsl_fit_linear(&voltage[max_rsquared], 1, &capacitance[max_rsquared], 1, length-1-max_rsquared, &c0, &c1, &c_cov00, &c_cov01, &c_cov11, &c_sumsq);
    fprintf(gnuPipe, "g(x) = %e + %e * x\n", c0, c1);


    // linear fit in slope
    gsl_fit_linear(&voltage[max_2_dev], 1, &capacitance[max_2_dev], 1, min_2_dev-max_2_dev, &d0, &d1, &d_cov00, &d_cov01, &d_cov11, &d_sumsq);
    fprintf(gnuPipe, "h(x) = %e + %e * x\n", d0, d1);

    // linear fit in slope of 1/C^2
    inv_cap_2_max = gsl_fit_linear(&voltage[max_2_dev], 1, &inv_cap_2[max_2_dev], 1, min_2_dev-max_2_dev, &f0, &f1, &f_cov00, &f_cov01, &f_cov11, &f_sumsq);
    fprintf(gnuPipe, "k(x) = %e + %e * x\n", f0, f1);

    // output read data to console for check
    for (int i = 0; i < length; i++)
    {
        printf("|%4d:  %.3lf V   %.3E A|\n", i, voltage[i], capacitance[i]);
        fprintf(fp2, "%.3E    %.3E    %.3E    %.3E    %.3E    %.3E\n", voltage[i], capacitance[i], capacitance_1_der[i], capacitance_2_der[i], rsquared[i], inv_cap_2[i]);
    }

    // Nbulk from 1/C^2
    Nbulk =  2.0/(f1*3.9*8.8541e-12*1.6e-19*pow(1.29e-3, 4.0));
    phi_ms = 4.08-(4.05+1.12/2.0-0.026*log(-Nbulk/1.45e-4));   
    
    // Vfb from diagram cross point of linear fits
    Vfb = 10e11*((c0-d0)/(10e11*(d1-c1)));
    fprintf(gnuPipe, "set arrow from %e, graph 0 to %e, graph 1 nohead\n", Vfb, Vfb);


    // Cox from the maximum
    Cox = gsl_stats_max(&capacitance[max_rsquared], 1, length-max_rsquared);


    // Tox Calculations from Cox = 3.9*e0*1.29e-3*1.29e-3/Cox
    Tox = 3.9*8.8541e-12*1.6641e-6/Cox;


    // Nox for given aluminum gate p-type silicon Na=5*10e12
    Nox = Cox*(-phi_ms-Vfb)/(-1.6e-19*pow(1.29e-3, 2.0));


    printf("c0:%E    c1:%E\nd0:%E    d1:%E\n", c0, c1, d0, d1);
    printf("voltage[max_rsquared]:%lf(V)    voltage[lenght]:%lf(V)\n",voltage[max_rsquared], voltage[length-1]);
    printf("max_rsquared:%d\n",max_rsquared);
    printf("Vfb:%lf(V)    Cox:%E(F)    Tox:%E(m)    Nox:%E(m^-2)    Nbulk:%E\n", Vfb, Cox, Tox, Nox, Nbulk);
    printf("Nbulk:%E    phi_ms:%E\n", Nbulk, phi_ms);


    {
        char buf[2048] = {};
        int n = 0;
        n = n + sprintf(&buf[n],"set terminal png size 800,800\n");
        n = n + sprintf(&buf[n],"set output '%s.png'\n", argv[1]);
        n = n + sprintf(&buf[n],"set xlabel \"Voltage(V)\" \nshow xlabel\n");
        n = n + sprintf(&buf[n],"set ylabel \"Capacitance(F)\"\nshow xlabel\n"); 
        n = n + sprintf(&buf[n],"set label 'y=Vfb' at %e+1, graph 0.5 font \"Source Code Pro Semibold\"\n", Vfb);
        n = n + sprintf(&buf[n],"set label \" Vfb:%.3lf(V) \\n Cox:%.3E(F) \\n Tox:%.3E(m) \\n Nox:%.3E(m^{-2})\" at graph 0.1, graph 0.8 font \"Source Code Pro Semibold\"\n",  Vfb, Cox, Tox, Nox);
        n = n + sprintf(&buf[n],"plot '%s.dat' u 1:2 with lines axes x1y1 linestyle 10 linewidth 1.5 linecolor 'grey30',", argv[1]);
        //n = n + sprintf(&buf[n],"'' u 1:6 with lines axes x1y2 linestyle 10 linewidth 1.5 linecolor 'grey40',", argv[1]);
        n = n + sprintf(&buf[n],"[%e:%e] g(x) linestyle 0 linewidth 3 linecolor 'red' title 'h(x)=%ex+%e',", Vfb, voltage[length-1], c1, c0);
        //n = n + sprintf(&buf[n],"[%e:%e] k(x) linestyle 0 linewidth 2 linecolor 'green' axis x1y2 title 'k(x)=%ex+%e',", voltage[max_2_dev], voltage[min_2_dev], f1, f0);
        n = n + sprintf(&buf[n],"[%e:%e] h(x) linestyle 0 linewidth 3 linecolor 'blue' title 'g(x)=%ex+%e'\n", Vfb, voltage[max_2_dev], d1, d0);                   
        //n = n + sprintf(&buf[n],"[:] [%e:%e] q(y) linestyle 0 linewidth 2 linecolor 'blue' title 'y=Vfb'\n", 0.0, gsl_stats_max(current, 1, length));

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