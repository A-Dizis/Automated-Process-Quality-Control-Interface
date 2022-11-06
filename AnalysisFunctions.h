int read_file(FILE** filename, FILE** plain_dat, char** argv_name)
{
    printf("%s\n", *argv_name);
    *filename = fopen(*argv_name, "r");
    {
        char buf[1024] = "";
        strcat(buf, *argv_name);
        strcat(buf, ".dat");
        *plain_dat = fopen(buf, "w");
    }
    return 0;
}

int ignore_lines(FILE** filename, int number_of_lines)
{
    char *ignore = (char *)malloc(1024 * sizeof(char));
    size_t ignore_size = 1024;
    if (ignore == NULL)
    {
        printf("fail to allocate ignore memory. exiting...\n");
    }
    for (int i = 0; i < number_of_lines; i++)
    {
        getline(&ignore, &ignore_size, *filename);
    }
    return 0;
}

int differentation(double** y, double** x, double** y_der, int lenght)
{
    *(*y_der+0) = (10e8 * (-3.0 * *(*y+0) - *(*y+2) + 4.0 * *(*y+1))) / (*(*x+2) - *(*x+0));
    *(*y_der+0) *= 10e-8;
    for (int i = 0; i < lenght - 2; i++)
    {
        *(*y_der+i+1) = (10e8 * (*(*y+i+2) - *(*y+i))) / (*(*x+i+2) - *(*x+i));
        *(*y_der+i+1) *= 10e-8;
    }
    *(*y_der+lenght-1) = (10e8 * (+3.0 * *(*y+lenght-1) + *(*y+lenght-3) - 4.0 * *(*y+lenght-2))) / (*(*x+lenght-1) - *(*x+lenght-3));
    *(*y_der+lenght-1) *= 10e-8;
    return 0;
}

int remove_dat_file(char** filename)
{
    char buf[2048] = "";
    strcat(buf, *filename);
    strcat(buf, ".dat");
    remove(buf);
    return 0;
}
