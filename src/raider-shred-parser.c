#include "raider-shred-parser.h"
#include "raider-pass-data.h"

/* ***************************************************************** */
/*                   Shred output is like this:                      */
/*                                                                   */
/*   shred: /home/pi/file-to-be-shredded.txt: pass 1/3 (random)...   */
/* ***************************************************************** */

struct _fsm
{
    void (*state)(void *);
    gchar **tokens;
    gint incremented_number;
    GtkWidget *progress_bar;
    gchar *filename;
};

void analyze_progress(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    struct _pass_data *pass_data = user_data;

    gchar *buf = g_data_input_stream_read_line_finish(pass_data->data_stream, res, NULL, NULL);

    /* If there is no data read in or available, return immediately. */
    if (buf == NULL)
    {
        return;
    }

    /* Parse the line of text. */

    /* The reason why I use a fsm style is because it allows redirects, and
    functions can be self contained, and not hold duplicate code. */

    gchar **tokens = g_strsplit(buf, " ", 0);
    struct _fsm fsm = {start, tokens, 0, pass_data->progress_bar, pass_data->filename};

    /* Pretty clever, no? */
    while (fsm.state != NULL)
    {
        fsm.state(&fsm);
    }

    g_free(tokens);
}

gboolean process_shred_output(gpointer data)
{
    /* Converting the stream to text. */
    struct _pass_data *pass_data = data;

    g_data_input_stream_read_line_async(pass_data->data_stream, G_PRIORITY_DEFAULT, NULL, analyze_progress, data);
    return TRUE;
}

/* Start the parsing. */
void start(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_sender_name;
}

/* Abort the while loop in analyze_progress. */
void stop(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = NULL;

    /* Reset the tokens array so it can be freed. */
    int i;
    for (i = 0; i < fsm->incremented_number; i++)
    {
        fsm->tokens--;
    }
}

/* This function checks if shred sent the output. */
void parse_sender_name(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_filename;

    if (g_strcmp0("/usr/bin/shred:", fsm->tokens[0]) != 0)
    {
        fsm->state = stop;
        g_printerr("No shred output found.");
        return;
    }

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

void parse_filename(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_pass;

    gchar **placeholder = g_strsplit(fsm->filename, " ", 0);

    /* This is for if the filename has multiple spaces in it. */
    int number = 0;
    while (placeholder[number] != NULL)
    {
        /* Point to the next word. */
        fsm->tokens++;
        fsm->incremented_number++;

        /* Update the number of spaces. */
        number++;
    }
    g_free(placeholder);
}

void parse_pass(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = parse_fraction;

    if (g_strcmp0("pass", fsm->tokens[0]) != 0)
    {
        fsm->state = stop;
        g_printerr("Could not find correct text (wanted 'pass', got '%s').", fsm->tokens[0]);
        return;
    }

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}

void parse_fraction(void *ptr_to_fsm)
{
    struct _fsm *fsm = ptr_to_fsm;
    fsm->state = stop;

    gchar **fraction_chars = g_strsplit(fsm->tokens[0], "/", 0);

    /* THE BIG CODE OF SHREDDING PROGRESS. */
    int current = g_strtod(fraction_chars[0], NULL);
    int number_of_passes = g_strtod(fraction_chars[1], NULL);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(fsm->progress_bar), (gdouble)current / number_of_passes);

    g_free(fraction_chars);

    /* Point to the next word. */
    fsm->tokens++;
    fsm->incremented_number++;
}