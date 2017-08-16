#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <gtk/gtk.h>
#include <pthread.h>

#include "sdlutils.h"
#include "random.h"

#define CHARS_X 180
#define CHARS_Y 50

#define WIDTH CHARS_X * CHAR_WIDTH
#define HEIGHT CHARS_Y * CHAR_HEIGHT

#define NEW_SIMULATION 1
#define QUIT 2
#define IMPORT_SAVE 3

#define ASK_RUN_METHOD_MESSAGE "Please enter run method, substituting %s for where the genes should be inserted: "

typedef enum
{
    STATE_MENU,
    STATE_IMPORTING_SAVE,
    STATE_CREATE_SIMULATION,
    STATE_SIMULATION_OPTIONS,
    STATE_SIMULATING
} state_t;

typedef struct
{
    double* genes;
    double fitness;
} genes_t;

state_t state;
char* run_method;
pthread_t run_thread;
double** gene_lists;
genes_t* genes_ts;
int population = 100;
int num_genes = 100; // Number of genes or product of generation #
int are_genes_constant = 1; // Is the number of genes constant?
int population_change = 100;
int num_genes_change = 100;
int generation_survivors = 10; // Top n survivors breed
int parents_per_child = 2;
int mutation_rate = 70; // 70% of first mutation, after first, 70% of second mutation, etc.
int generation_number;
int was_loaded = 0;
int starting_generation;
int generations_to_run = -1;
int loading = 0; // Is it loading a new generation?
int stop; // Used to communicate with the simulating thread
int batch_genes = 0; // Can the program process multiple gene sets (a whole generation) at once?



void draw_simulation_options()
{
    char* popbuffer = malloc(12);
    char* popchangebuffer = malloc(12);
    char* ppcbuffer = malloc(12);
    char* topnbuffer = malloc(12);
    char* mutratebuffer = malloc(12);
    char* genesbuffer = malloc(12);

    write_str("Number of genes is ", 1, 3, WHITE, BLACK);
    write_str("Linear to the generation number (n)", 20, 3, !are_genes_constant ? LIGHTBLUE : WHITE, BLACK);
    write_str("/", 55, 3, WHITE, BLACK);
    write_str("Constant (c)", 56, 3, are_genes_constant ? LIGHTBLUE : WHITE, BLACK);
    char* genes_eq = are_genes_constant ? "Genes = " : "Genes = generation number * ";
    int len = strlen(genes_eq);
    write_str("                                                                      ", 1, 4, BLACK, BLACK);
    write_str(genes_eq, 1, 4, WHITE, BLACK);
    sprintf(genesbuffer, "%12d", num_genes);
    write_str(genesbuffer, 1+len, 4, WHITE, BLACK);
    write_str("-1 (1)", 14+len, 4, LIGHTRED, DARKRED);
    write_str("+1 (2)", 20+len, 4, LIGHTGREEN, DARKGREEN);
    write_str("/2 (3)", 26+len, 4, LIGHTRED, DARKRED);
    write_str("*2 (4)", 32+len, 4, LIGHTGREEN, DARKGREEN);

    write_str("Population: ", 1, 5, WHITE, BLACK);
    sprintf(popbuffer, "%12d", population);
    write_str(popbuffer, 13, 5, WHITE, BLACK);
    write_str("- (l)", 26, 5, LIGHTRED, DARKRED);
    write_str("+ (p)", 32, 5, LIGHTGREEN, DARKGREEN);
    write_str("(Increase by)", 37, 5, WHITE, BLACK);
    sprintf(popchangebuffer, "%12d", population_change);
    write_str(popchangebuffer, 50, 5, WHITE, BLACK);
    write_str("- (k)", 62, 5, LIGHTRED, DARKRED);
    write_str("+ (o)", 68, 5, LIGHTGREEN, DARKGREEN);

    write_str("Parents per child: ", 1, 6, WHITE, BLACK);
    sprintf(ppcbuffer, "%3d", parents_per_child);
    write_str(ppcbuffer, 20, 6, WHITE, BLACK);
    write_str("- (f)", 24, 6, LIGHTRED, DARKRED);
    write_str("+ (r)", 30, 6, LIGHTGREEN, DARKGREEN);

    write_str("Top n algorithms survive: ", 1, 7, WHITE, BLACK);
    sprintf(topnbuffer, "%12d", generation_survivors);
    write_str(topnbuffer, 27, 7, WHITE, BLACK);
    write_str("- (g)", 39, 7, LIGHTRED, DARKRED);
    write_str("+ (t)", 45, 7, LIGHTGREEN, DARKGREEN);
    write_str("/2 (v)", 51, 7, RED, DARKRED);
    write_str("*2 (b)", 57, 7, GREEN, DARKGREEN);

    write_str("Mutation rate: ", 1, 8, WHITE, BLACK);
    sprintf(mutratebuffer, "%3d", mutation_rate);
    write_str(mutratebuffer, 16, 8, WHITE, BLACK);
    write_str("- (h)", 20, 8, LIGHTRED, DARKRED);
    write_str("+ (y)", 26, 8, LIGHTGREEN, DARKGREEN);

    write_str("Batch genes? ", 1, 9, WHITE, BLACK);
    write_str("ON (z)", 14, 9, batch_genes ? LIGHTBLUE : WHITE, BLACK);
    write_str("OFF (x)", 21, 9, !batch_genes ? LIGHTBLUE : WHITE, BLACK);
    write_str("If you enable batch genes, the input will be provided as semicolon-separated comma-separated lists of genes. This will improve run time.",
                1, 10, WHITE, BLACK);
    write_str("Begin (Return)", 1, 12, LIGHTGREEN, DARKGREEN);
}

int get_num_genes()
{
    return are_genes_constant ? num_genes : (num_genes * generation_number);
}

void allocate_genes()
{
    int i;
    int num_genes = get_num_genes(generation_number);
    for (i = 0; i < population; i++)
        gene_lists[i] = malloc(num_genes * sizeof(double));
}

double get_fitness(double* genes)
{
    int ngenes = get_num_genes();
    char* csgenes = malloc(ngenes * 16); // Comma-separated genes
    csgenes[0] = 0;
    strcat(csgenes, double_to_str(genes[0]));
    int i;
    for (i = 1; i < ngenes; i++)
    {
        strcat(csgenes, ",");
        strcat(csgenes, double_to_str(genes[i]));
    }
    char* command = malloc(ngenes * 16 + 64);
    sprintf(command, run_method, csgenes);
    FILE* fp = popen(command, "r");
    float fitness;
    fscanf(fp, "%f", &fitness);
    pclose(fp);
    return (double) fitness;
}

double* get_fitnesses(double** genes)
{
    int i, j, ngenes = get_num_genes();
    char* buffer = malloc(ngenes * population * 12);
    strcat(buffer, double_to_str(genes[0][0]));
    for (i = 1; i < ngenes; i++)
    {
        strcat(buffer, ",");
        strcat(buffer, double_to_str(genes[0][i]));
    }
    for (j = 1; j < population; j++)
    {
        strcat(buffer, ";");
        strcat(buffer, double_to_str(genes[j][0]));
        for (i = 1; i < ngenes; i++)
        {
            strcat(buffer, ",");
            strcat(buffer, double_to_str(genes[j][i]));
        }
    }
    char* command = malloc(strlen(buffer) + strlen(run_method) + 50);
    sprintf(command, run_method, buffer);
    FILE* fp = popen(command, "r");
    double* fitnesses;
    float f;
    fitnesses = malloc(population * sizeof(double));
    fscanf(fp, "%f", &f);
    fitnesses[0] = (double) f;
    for (i = 1; i < population; i++)
    {
        fscanf(fp, ",%f", &f);
        fitnesses[i] = (double) f;
    }
    fclose(fp);
    return fitnesses;
}

void mutate(double* genes)
{
    int ngenes = get_num_genes();
    int selected_gene;
    while (rand01() < ((double) mutation_rate / 100))
    {
        selected_gene = randrange_int(0, ngenes);
        genes[selected_gene] = rand01();
    }
}

double* breed(double** genes)
{
    int i, newngenes, ngenes = get_num_genes();

    generation_number++;
    newngenes = get_num_genes();
    generation_number--;

    double* newgenes = malloc(newngenes*sizeof(double));

    for (i = 0; i < ngenes; i++)
    {
        newgenes[i] = genes[randrange_int(0, parents_per_child)][i];
    }
    for (; i < newngenes; i++)
    {
        newgenes[i] = rand01();
    }
    mutate(newgenes);
    return newgenes;
}

double** select_parents(genes_t* sorted_genes)
{
    int* selected = malloc(parents_per_child*sizeof(int));
    int i, j, hasBeenSelected, sel;
    for (i = 0; i < parents_per_child; i++)
    {
        do
        {
            sel = randrange_int(0, generation_survivors);
            hasBeenSelected = 0;
            for (j = 0; j < i; j++)
            {
                if (sel == selected[j])
                    hasBeenSelected = 1;
            }
        }
        while (hasBeenSelected);
        selected[i] = sel;
    }
    double** parents = malloc(parents_per_child*sizeof(double*));
    for (i = 0; i < parents_per_child; i++)
    {
        parents[i] = sorted_genes[selected[i]].genes;
    }
    return parents;
}

int compare_genes(const void* a, const void* b)
{
    genes_t genes_a = *(genes_t*) a;
    genes_t genes_b = *(genes_t*) b;
    double diff = genes_b.fitness - genes_a.fitness;
    return diff > 0 ? 1 : (diff == 0 ? 0 : -1);
}



void* run_generations(void* data)
{
    // Runs generations_to_run generations
    if (loading) return NULL;
    loading = 1;
    stop = 0;
    int i, gn; // Number of local generations (out of n)
    starting_generation = generation_number;
    double* fitnesses;
    for (gn = 0; generations_to_run == -1 ? 1 : (gn < generations_to_run); gn++)
    {
        if (batch_genes)
        {
            fitnesses = get_fitnesses(gene_lists);
            for (i = 0; i < population; i++)
            {
                genes_ts[i].genes = gene_lists[i];
                genes_ts[i].fitness = fitnesses[i];
            }
        }
        else
        {
            for (i = 0; i < population; i++)
            {
                genes_ts[i].genes = gene_lists[i];
                genes_ts[i].fitness = get_fitness(genes_ts[i].genes);
            }
        }
        qsort(genes_ts, population, sizeof(genes_t), compare_genes);
        if (stop)
        {
            loading = -1;
            return NULL;
        }
        for (i = 0; i < population; i++)
        {
            gene_lists[i] = breed(select_parents(genes_ts));
        }
        printf("Generation %d; Best fitness: %f\n", generation_number, genes_ts[0].fitness);
        generation_number++;
        if (stop)
        {
            loading = -1;
            return NULL;
        }
    }

    loading = -1;
    generations_to_run = -1;
    return NULL;
}

void simulate()
{
    int i, j, num_genes;
    if (!was_loaded)
    {
        generation_number = 1;
        gene_lists = malloc(population * sizeof(double*));
        num_genes = get_num_genes();
        allocate_genes();
        for (i = 0; i < population; i++)
            for (j = 0; j < num_genes; j++)
                gene_lists[i][j] = rand01();
    }

    write_str("Run 1 generation (1)", 1, 1, WHITE, DARKBLUE);
    write_str("Run 10 generations (2)", 23, 1, WHITE, DARKBLUE);
    write_str("Run 100 generations (3)", 47, 1, WHITE, DARKBLUE);
    write_str("Run 1000 generations (4)", 72, 1, WHITE, DARKBLUE);
    write_str("Run 10000 generations (5)", 98, 1, WHITE, DARKBLUE);
    write_str("Keep running generations (6)", 125, 1, WHITE, DARKBLUE);
    write_str("Stop (7)", 155, 1, WHITE, DARKBLUE);
}

void change_state(state_t to)
{
    state = to;
    clear_options();
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    switch (to)
    {
    case STATE_IMPORTING_SAVE:
        write_str("Please enter the path to the save file: ", 1, 1, WHITE, BLACK);
        setup_entry("", 41, 1);
        break;
    case STATE_CREATE_SIMULATION:
        setup_entry("", strlen(ASK_RUN_METHOD_MESSAGE)+1, 1);
        write_str(ASK_RUN_METHOD_MESSAGE, 1, 1, WHITE, BLACK);
        write_str("When this command is run, %s will be replaced with a comma-separated list of genes.", 1, 2, WHITE, BLACK);
        break;
    case STATE_SIMULATION_OPTIONS:
        hcenter_str("Simulation Options", CHARS_X, 1, WHITE, BLACK);
        draw_simulation_options();
        break;
    case STATE_SIMULATING:
        simulate();
        break;
    }
}

void save(char* filename)
{
    int i, j;
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "%d\n", generation_number);
    fprintf(fp, "%s\n", run_method);
    fprintf(fp, "%d\n", population);
    fprintf(fp, "%d\n", num_genes);
    fprintf(fp, "%d\n", are_genes_constant);
    fprintf(fp, "%d\n", parents_per_child);
    fprintf(fp, "%d\n", generation_survivors);
    fprintf(fp, "%d\n", mutation_rate);
    fprintf(fp, "%d\n", batch_genes);
    for (i = 0; i < population; i++)
    {
        for (j = 0; j < get_num_genes(); j++)
            fprintf(fp, "%f ", genes_ts[i].genes[j]);
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void load(char* filename)
{
    int i, j;
    FILE* fp = fopen(filename, "r");
    run_method = malloc(512);
    fscanf(fp, "%d\n", &generation_number);
    fscanf(fp, "%[^\n]s\n", run_method);
    fscanf(fp, "%d\n", &population);
    fscanf(fp, "%d\n", &num_genes);
    fscanf(fp, "%d\n", &are_genes_constant);
    fscanf(fp, "%d\n", &parents_per_child);
    fscanf(fp, "%d\n", &generation_survivors);
    fscanf(fp, "%d\n", &mutation_rate);
    fscanf(fp, "%d\n", &batch_genes);
    float f;
    gene_lists = malloc(population * sizeof(double*));
    allocate_genes();
    for (i = 0; i < population; i++)
    {
        for (j = 0; j < get_num_genes(); j++)
        {
            fscanf(fp, "%f ", &f);
            gene_lists[i][j] = f;
        }
        fscanf(fp, "\n");
    }
    fclose(fp);
    was_loaded = 1;
    change_state(STATE_SIMULATING);
}

int main()
{
    srand(time(NULL));
    int end = 0;
    SDL_Event event;

    sdlutils_init(WIDTH, HEIGHT);
    sdlutils_set_capacity(256);

    // Title
    write_str("Genetic Algorithm Framework", 1, 1, WHITE, BLACK);
    create_option("New simulation", NEW_SIMULATION, 1, 2, LIGHTGREEN, DARKGREEN, rgba(100, 200, 100, 255));
    create_option("Import from save file", IMPORT_SAVE, 1, 3, LIGHTBLUE, DARKBLUE, rgba(100, 100, 200, 255));
    create_option("Quit", QUIT, 1, 4, LIGHTRED, DARKRED, rgba(200, 100, 100, 255));
    draw_options();
    write_str("See github.com/pommicket/genetic for more information.", 1, 5, WHITE, BLACK);

    int action;


    genes_ts = malloc(population * sizeof(genes_t));

    char* filename;
    int i, j, r1, r2;
    FILE* fp;
    char* buffer;
    char* gen = malloc(256);
    int lgnum = -1;
    int lgtrun = -1;
    while (!end)
    {
        if (state == STATE_SIMULATING && (lgnum != generation_number || lgtrun != generations_to_run))
        {
            if (generations_to_run > 0)
                sprintf(gen, "      Generation %12d of %12d      ", generation_number, starting_generation + generations_to_run);
            else
                sprintf(gen, "            Generation %12d             ", generation_number);
            lgnum = generation_number;
            lgtrun = generations_to_run;
            hcenter_str(gen, CHARS_X, 4, WHITE, BLACK);
        }
        if (loading == -1)
        {
            // When done loading
            write_str("          ", 1, 2, BLACK, BLACK);
            write_str("Export best set of genes (b)", 1, 3, WHITE, DARKGREEN);
            write_str("Export generation (g)", 31, 3, WHITE, DARKGREEN);
            write_str("Save progress (s)", 54, 3, WHITE, DARKGREEN);
            buffer = malloc(32);
            write_str("Best fitness: ", 1, 4, WHITE, BLACK);
            sprintf(buffer, "%f", genes_ts[0].fitness);
            write_str(buffer, 15, 4, WHITE, BLACK);
            write_str("Genes:", 1, 5, WHITE, BLACK);
            for (i = 0; i < get_num_genes() && i < CHARS_Y; i++)
            {
                sprintf(buffer, "%f", genes_ts[0].genes[i]);
                write_str(buffer, 1, i + 6, WHITE, BLACK);
            }
            loading = 0;

        }
        SDL_RenderPresent(renderer);
        while (SDL_PollEvent(&event))
        {

            switch (event.type)
            {
            case SDL_QUIT:
                end = 1;
                break;
            case SDL_KEYDOWN:
                switch (state)
                {
                case STATE_SIMULATION_OPTIONS:
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_p:
                        if (population + population_change > 0) population += population_change;
                        break;
                    case SDLK_l:
                        if (population - population_change > 0) population -= population_change;
                        break;
                    case SDLK_o:
                        if (population_change * 2 > 0) population_change *= 2;
                        break;
                    case SDLK_k:
                        if (population_change / 2 > 0) population_change /= 2;
                        break;
                    case SDLK_n:
                        are_genes_constant = 0;
                        break;
                    case SDLK_c:
                        are_genes_constant = 1;
                        break;
                    case SDLK_f:
                        if (parents_per_child > 1) parents_per_child--;
                        break;
                    case SDLK_r:
                        if (parents_per_child < generation_survivors) parents_per_child++;
                        break;
                    case SDLK_g:
                        if (generation_survivors > 1) generation_survivors--;
                        break;
                    case SDLK_t:
                        if (generation_survivors < population) generation_survivors++;
                        break;
                    case SDLK_v:
                        if (generation_survivors / 2 >= 1) generation_survivors /= 2;
                        break;
                    case SDLK_b:
                        if (generation_survivors * 2 <= population) generation_survivors *= 2;
                        break;
                    case SDLK_h:
                        if (mutation_rate) mutation_rate--;
                        break;
                    case SDLK_y:
                        if (mutation_rate < 100) mutation_rate++;
                        break;
                    case SDLK_z:
                        batch_genes = 1;
                        break;
                    case SDLK_x:
                        batch_genes = 0;
                        break;
                    case SDLK_1:
                        if (num_genes > 0) num_genes--;
                        break;
                    case SDLK_2:
                        num_genes++;
                        break;
                    case SDLK_3:
                        num_genes /= 2;
                        break;
                    case SDLK_4:
                        num_genes *= 2;
                        break;
                    case SDLK_RETURN:
                        change_state(STATE_SIMULATING);
                        break;
                    }
                    if (state == STATE_SIMULATION_OPTIONS) draw_simulation_options();
                    break;
                case STATE_SIMULATING:
                    generations_to_run = 0;
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_1:
                        generations_to_run = 1;
                        break;
                    case SDLK_2:
                        generations_to_run = 10;
                        break;
                    case SDLK_3:
                        generations_to_run = 100;
                        break;
                    case SDLK_4:
                        generations_to_run = 1000;
                        break;
                    case SDLK_5:
                        generations_to_run = 10000;
                        break;
                    case SDLK_6:
                        generations_to_run = -1;
                        break;
                    case SDLK_7:
                        stop = 1;
                        loading = -1;
                        break;
                    case SDLK_b:
                        if (loading) break;
                        filename = malloc(64);
                        r1 = rand() % 100000;
                        r2 = rand() % 100000;
                        sprintf(filename, "genes%05d%05d.txt", r1, r2);
                        fp = fopen(filename, "w");
                        for (i = 0; i < get_num_genes(); i++)
                            fprintf(fp, "%f\n", genes_ts[0].genes[i]);
                        fclose(fp);
                        write_str("Exported to ", 73, 3, WHITE, BLACK);
                        write_str(filename, 85, 3, WHITE, BLACK);
                        write_str("            ", 104, 3, BLACK, BLACK);
                        break;
                    case SDLK_g:
                        if (loading) break;
                        filename = malloc(64);
                        r1 = rand() % 100000;
                        r2 = rand() % 100000;
                        sprintf(filename, "generation%05d%05d.txt", r1, r2);
                        fp = fopen(filename, "w");
                        for (i = 0; i < population; i++){
                            for (j = 0; j < get_num_genes(); j++)
                                fprintf(fp, "%f ", genes_ts[i].genes[j]);
                            fprintf(fp, "\n");
                        }
                        fclose(fp);
                        write_str("Exported to ", 73, 3, WHITE, BLACK);
                        write_str(filename, 85, 3, WHITE, BLACK);
                        break;
                    case SDLK_s:
                        if (loading) break;
                        filename = malloc(64);
                        r1 = rand() % 10000;
                        r2 = rand() % 10000;
                        sprintf(filename, "save%04d%04d.save", r1, r2);
                        save(filename);
                        write_str("Saved to ", 73, 3, WHITE, BLACK);
                        write_str(filename, 82, 3, WHITE, BLACK);
                        write_str("              ", 99, 3, BLACK, BLACK);
                        break;
                    }
                    if (generations_to_run)
                    {
                        write_str("Loading...", 1, 2, WHITE, BLACK);
                        pthread_create(&run_thread, NULL, run_generations, NULL);
                    }
                    break;
                }
                action = sdlutils_process_key(event.key.keysym.sym);
                if (action)
                {
                    switch (action)
                    {
                    case NEW_SIMULATION:
                        change_state(STATE_CREATE_SIMULATION);
                        break;
                    case ENTRY_SUBMIT:
                        switch (state)
                        {
                        case STATE_CREATE_SIMULATION:
                            run_method = malloc(512);
                            strcpy(run_method, get_entry_contents());
                            change_state(STATE_SIMULATION_OPTIONS);
                            break;
                        case STATE_IMPORTING_SAVE:
                            buffer = malloc(512);
                            strcpy(buffer, get_entry_contents());
                            load(buffer);
                            break;
                        }
                        break;
                    case QUIT:
                        end = 1;
                        break;
                    case IMPORT_SAVE:
                        change_state(STATE_IMPORTING_SAVE);
                        break;
                    }
                }
                break;
            case SDL_TEXTINPUT:
                handle_text(event.text.text);
            }

        }
    }

    quit();
    return 0;
}
