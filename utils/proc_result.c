#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

double sample_mean(double *xi, int b, int e);
double unbiased_sample_variance(double *xi, double x_mean, int b, int e);
    
enum {
    CNT_WORDS_PER_LINE = 20,
    XI_OUTLIERS_BEGIN = 5,
    XI_OUTLIERS_END = 14,
    MAX_STR_SIZE = 1000,
};

void process_line(char *line, char **fnames)
{
    fnames[0] = line;
    for (int i = 1; *line != '\n'; line++) {
        if (*line == ' ') {
            *line = '\0';
            line++;
            if (*line != '\n') {
                fnames[i] = line;
                i++;
            }
        }
    }
    *line = '\0';
}

inline double sample_mean(double *xi, int b, int e)
{
    double mean = 0;

    for (int i = b; i <= e; i++) {
        mean += xi[i];
    }
    mean = mean / (double)(e - b + 1.0);
    
    return mean;
}

inline double unbiased_sample_variance(double *xi, double x_mean, int b, int e)
{
    double n = (double) (e - b + 1.0);
    double variance = 0;

    for (int i = b; i <= e; i++) {
        variance += pow((xi[i] - x_mean), 2.0);
    }
    variance /= (n - 1.0);

    return variance;
}

int compare_doubles(const void *a, const void *b)
{
    const double *da = (const double *) a;
    const double *db = (const double *) b;

    return (*da > *db) - (*da < *db);
}

void process_result(char **res_names) {
    FILE *fres = NULL;
    double xi[CNT_WORDS_PER_LINE];
    double x_mean, variance, std_dev, std_err, delta_x;
    double n = (double)(XI_OUTLIERS_END - XI_OUTLIERS_BEGIN + 1.0);
    double t_p_n = 3.250; /* p = 0.99; n = 10 */

    for (int i = 0; i < CNT_WORDS_PER_LINE; i++) {
        fres = fopen(res_names[i], "r");
        if (fres == NULL) {
            printf("no result file [%s]\n", res_names[i]);
            exit(EXIT_FAILURE);
        }
        fscanf(fres, "time %lf", &xi[i]);
//        printf("%s %lf\n", res_names[i], xi[i] );
        fclose(fres);
    }
    qsort(xi, CNT_WORDS_PER_LINE, sizeof(double), compare_doubles);
    x_mean = sample_mean(xi, XI_OUTLIERS_BEGIN, XI_OUTLIERS_END);
    printf("sample mean = %lf\n", x_mean);
    variance = unbiased_sample_variance(xi, x_mean, 
                                        XI_OUTLIERS_BEGIN, XI_OUTLIERS_END);
    std_dev = sqrt(variance);
    std_err = std_dev / sqrt(n);
    delta_x = std_err * t_p_n;
    printf("x = %lf +- %lf\n", x_mean, delta_x);
    printf("x = %lf +- %lf * %lf\n", x_mean, std_err, t_p_n);
}

int main(int argc, char **argv)
{
    char *map_name = NULL;
    char line_buf[MAX_STR_SIZE];
    char *res_names[CNT_WORDS_PER_LINE];
    FILE *map = NULL;

    if (argc < 2 || (map_name = argv[1]) == NULL) {
        printf("usage: ./proc_results.out <results map>\n");
        exit(EXIT_FAILURE);
    }

    map = fopen(map_name, "r");
    if (map == NULL) {
        printf("no results mup file\n");
        exit(EXIT_FAILURE);
    }

    while (fgets(line_buf, MAX_STR_SIZE, map)) {
        memset(res_names, 0, CNT_WORDS_PER_LINE * sizeof(char *));
        process_line(line_buf, res_names);
        for (int i = 0; i < CNT_WORDS_PER_LINE; i++) {
            printf("%s ", res_names[i]);
        }
        printf("\n");
        process_result(res_names);
        memset(line_buf, 0, MAX_STR_SIZE);
    }

    fclose(map);
    
    return 0;
}
