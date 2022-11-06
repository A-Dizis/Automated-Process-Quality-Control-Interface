#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

GtkWidget *window1;
GtkWidget *fixed1;
GtkWidget *button1;
GtkWidget *analysis;
GtkWidget *measurement;
GtkWidget *diodeiv;
GtkWidget *diodecv;
GtkWidget *moscv;
GtkWidget *gcdiv;
GtkWidget *fourwireiv;
GtkWidget *fetiv;
GtkWidget *dielectricbreakdowniv;
GtkWidget *couplingcapacitancecv;

// Console log declarations
GtkWidget *bufferscrolledwindow;
GtkWidget *bufferviewport;
GtkWidget *consolelog;
GtkTextIter start;
GtkTextIter *start_ptr = &start;

//Open Files log
GtkWidget *openfilesscrolledwindow;
GtkWidget *openfilesviewport;
GtkWidget *openfileslog;
GtkTextIter start_openfiles;
GtkTextIter *start_openfiles_ptr = &start;
GtkTextIter end;
GtkTextIter *end_openfiles_ptr = &end;



GtkWidget *filechooser;
GtkWidget *dialog;
GtkWidget *showbutton;


time_t rawtime;
struct tm* timeinfo;

GtkBuilder *builder;

GtkTextBuffer *buffer;
GtkTextBuffer *openfilesbuffer;

FILE *analysisPipe = NULL;
//char filename[256] = "";

GSList *filenames = NULL;
int method_to_call = 0;

int main(int argc, char *argv[]) {
    
    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file("simple.glade");

    window1 = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));

    g_signal_connect(window1, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_builder_connect_signals(builder, NULL);
    
    fixed1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
    analysis = GTK_WIDGET(gtk_builder_get_object(builder, "analysis"));
    measurement = GTK_WIDGET(gtk_builder_get_object(builder, "measurement"));
    diodeiv = GTK_WIDGET(gtk_builder_get_object(builder, "diodeiv"));
    diodecv = GTK_WIDGET(gtk_builder_get_object(builder, "diodecv"));
    moscv = GTK_WIDGET(gtk_builder_get_object(builder, "moscv"));
    gcdiv = GTK_WIDGET(gtk_builder_get_object(builder, "gcdiv"));
    fourwireiv = GTK_WIDGET(gtk_builder_get_object(builder, "fourwireiv"));
    fetiv = GTK_WIDGET(gtk_builder_get_object(builder, "fetiv"));
    dielectricbreakdowniv = GTK_WIDGET(gtk_builder_get_object(builder, "dielectricbreakdowniv"));
    couplingcapacitancecv = GTK_WIDGET(gtk_builder_get_object(builder, "couplingcapacitancecv"));
    
	
	bufferscrolledwindow = GTK_WIDGET(gtk_builder_get_object(builder, "bufferscrolledwindow"));
    bufferviewport = GTK_WIDGET(gtk_builder_get_object(builder, "bufferviewport"));
    consolelog = GTK_WIDGET(gtk_builder_get_object(builder, "consolelog"));

	openfilesscrolledwindow = GTK_WIDGET(gtk_builder_get_object(builder, "openfilesscrolledwindow"));
    openfilesviewport = GTK_WIDGET(gtk_builder_get_object(builder, "openfilesviewport"));
    openfileslog = GTK_WIDGET(gtk_builder_get_object(builder, "openfileslog"));

    filechooser = GTK_WIDGET(gtk_builder_get_object(builder, "filechooser"));
    showbutton = GTK_WIDGET(gtk_builder_get_object(builder, "showbutton"));
    
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(consolelog), 0);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(openfileslog), 0);
    gtk_widget_show(window1);

    gtk_main();

    return 0;

}
 
// Analysis button
void on_analysis_clicked(GtkButton *b) {
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    GSList *temp = filenames;


    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(consolelog));
	gtk_text_buffer_get_iter_at_line(buffer, start_ptr, (gint)0);

    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"\n", (gint)-1);
    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)asctime(timeinfo), (gint)-1);
    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Starting analysis ...\n", (gint)-1);
    if(filenames == NULL){
	printf("No file chosen\n");
	gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Error: No file(s) chosen\n", (gint)-1);
    }
    else{
	char minibuf[1024]="";
	
	switch(method_to_call){
	case 0:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"No method chosen ...\n", (gint)-1);
	    break;

	case 1:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Diode IV analysis ...\n", (gint)-1);
	    while(temp != NULL)
		{
		    strcat(minibuf, "./DIODE-IV \"");
		    strcat(minibuf, (const gchar*)temp->data);
		    strcat(minibuf, "\" ");
		    analysisPipe = popen(minibuf, "w");	    
		    temp = temp->next;
		    strcpy(minibuf, "");
		}
	    
	    pclose(analysisPipe);

	    break;
	    
	case 2:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Diode CV analysis ...\n", (gint)-1);
	    while(temp != NULL)
		{
		    strcat(minibuf, "./DIODE-CV \"");
		    strcat(minibuf, (const gchar*)temp->data);
		    strcat(minibuf, "\" ");
		    analysisPipe = popen(minibuf, "w");	    
		    temp = temp->next;
		    strcpy(minibuf, "");
		}
	    
	    pclose(analysisPipe);
	    break;

	case 3:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"MOS CV analysis ...\n", (gint)-1);

	    while(temp != NULL)
		{
		    strcat(minibuf, "./MOS-CV \"");
		    strcat(minibuf, (const gchar*)temp->data);
		    strcat(minibuf, "\" ");
		    analysisPipe = popen(minibuf, "w");	    
		    temp = temp->next;
		    strcpy(minibuf, "");
		}	    
	    pclose(analysisPipe);	    
	    break;

	case 4:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"GCD IV analysis ...\n", (gint)-1);
		while(temp != NULL)
		{
		    strcat(minibuf, "./GCD-IV \"");
		    strcat(minibuf, (const gchar*)temp->data);
		    strcat(minibuf, "\" ");
		    analysisPipe = popen(minibuf, "w");	    
		    temp = temp->next;
		    strcpy(minibuf, "");
		}
	    break;

	case 5:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"4-Wire IV ...\n", (gint)-1);
		while(temp != NULL)
		{
		    strcat(minibuf, "./FOURWIRE-IV \"");
		    strcat(minibuf, (const gchar*)temp->data);
		    strcat(minibuf, "\" ");
		    analysisPipe = popen(minibuf, "w");	    
		    temp = temp->next;
		    strcpy(minibuf, "");
		}
	    break;

	case 6:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"FET IV analysis ...\n", (gint)-1);
	    while(temp != NULL)
		{
		    strcat(minibuf, "./FET-IV \"");
		    strcat(minibuf, (const gchar*)temp->data);
		    strcat(minibuf, "\" ");
		    analysisPipe = popen(minibuf, "w");	    
		    temp = temp->next;
		    strcpy(minibuf, "");
		}
	    pclose(analysisPipe);
	    break;

	case 7:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Dielectric Breakdown IV analysis ...\n", (gint)-1);
	    break;
	    
	case 8:
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Coupling Capacitance CV analysis ...\n", (gint)-1);
	    while(temp != NULL)
		{
		    strcat(minibuf, "./COUPLING-CAP \"");
		    strcat(minibuf, (const gchar*)temp->data);
		    strcat(minibuf, "\" ");
		    analysisPipe = popen(minibuf, "w");	    
		    temp = temp->next;
		    strcpy(minibuf, "");
		}
	    pclose(analysisPipe);
	    break;
	}

	gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"\n", (gint)-1);
	consolelog = gtk_text_view_new_with_buffer(buffer);
	
    }
}


// Radio Buttons
void on_diodeiv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 1;
}
void on_diodecv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 2;
}
void on_moscv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 3;
}
void on_gcdiv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 4;

}
void on_fourwireiv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 5;
}
void on_fetiv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 6;
}
void on_dielectricbreakdowniv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 7;
}
void on_couplingcapacitancecv_toggled(GtkRadioButton *b){

    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    if(T) method_to_call = 8;   
}



// Open File Operation
void on_fileopenbutton_clicked(GtkButton *b)
{
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    int i = 0;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Open File",
					  NULL,
					  action,
					  "Cancel",
					  GTK_RESPONSE_CANCEL,
					  "_Open",
					  GTK_RESPONSE_ACCEPT,
					  NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    res = gtk_dialog_run (GTK_DIALOG (dialog));
    if (res == GTK_RESPONSE_ACCEPT)
	{
	    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	    filenames = gtk_file_chooser_get_filenames (chooser);
	}

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(consolelog));
	openfilesbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(openfileslog));

	gtk_text_buffer_get_iter_at_offset(openfilesbuffer, start_openfiles_ptr, (gint)0);
	gtk_text_buffer_get_iter_at_offset(openfilesbuffer, end_openfiles_ptr, (gint)-1);
	gtk_text_buffer_delete(openfilesbuffer, start_openfiles_ptr, end_openfiles_ptr);

	gtk_text_buffer_get_iter_at_line(buffer, start_ptr, (gint)0);

    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"\n", (gint)-1);
    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)asctime(timeinfo), (gint)-1);
    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Opening ... ", (gint)-1);

    GSList *temp = filenames;
	int file_number = 1;
	char file_number_str[100] = "";
    while(temp != NULL)
	{
		strcpy(file_number_str, "");
		sprintf(file_number_str, "(%d.)  -  ", file_number);
	    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)temp->data, (gint)-1);
		gtk_text_buffer_insert_at_cursor (openfilesbuffer, (const gchar*) file_number_str, (gint)-1);
		gtk_text_buffer_insert_at_cursor (openfilesbuffer, (const gchar*)temp->data, (gint)-1);
		gtk_text_buffer_insert_at_cursor (openfilesbuffer, (const gchar*)"\n", (gint)-1);
	    printf("%s\n\n", (const gchar*)temp->data);
	    temp = temp->next;
		file_number++;
	}

    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"\n", (gint)-1);
    
	
	consolelog = gtk_text_view_new_with_buffer(buffer);
	openfileslog = gtk_text_view_new_with_buffer(openfilesbuffer);
    
    gtk_widget_destroy (dialog);
}

// Show plots

void on_showbutton_clicked(GtkButton *b)
{
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    
    char minibuf[10000]="";
    FILE *webpage = NULL;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(consolelog));
	gtk_text_buffer_get_iter_at_line(buffer, start_ptr, (gint)0);

    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"\n", (gint)-1);
    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)asctime(timeinfo), (gint)-1);
    gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Plot(s) will be previewed in Firefox ...\n", (gint)-1);
    
    GSList *temp = filenames;

    strcat(minibuf,"<!DOCTYPE html><html lang=\"\"><head><meta charset=\"utf-8\"><title>Plottings</title></head><body>");	   
    while(temp != NULL)
	{
	    strcat(minibuf, "<img src=\"");
	    strcat(minibuf, (const char *)temp->data);
	    strcat(minibuf, ".png\" style=\"height: 350px; width: 350px; margin: 10px;\"></img>\n");

	    temp = temp->next;
	}
    strcat(minibuf, "</body></html>");

    if(filenames!=NULL)
	{
	    webpage = fopen("webshow.html", "w");
	    fprintf(webpage, "%s", minibuf);
	    printf("%s\n", minibuf);
	    fclose(webpage);

	    webpage = popen("firefox webshow.html", "w");
	    pclose(webpage);
	}
    else
	gtk_text_buffer_insert(buffer, start_ptr,(const gchar*)"Error: You have not chosen file(s) ...\n", (gint)-1);

    consolelog = gtk_text_view_new_with_buffer(buffer);

}
