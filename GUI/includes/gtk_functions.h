#ifndef GTK_FUNCTIONS_H
#define GTK_FUNCTIONS_H

#include <gtk/gtk.h>

/******** GTK **********/
//MAINW
void activate(GtkApplication *app, gpointer user_data);

//FASTW
void fastWindow(void);

//SAVEW
void saveWindow();
int saveUser(const char *username, int settings); //json handle
//LOADW
void loadWindow();
struct userData *getUserData(void);

//SLIDEW
void sliderValueChanged(GtkRange *range, gpointer user_data);
void sliderWindow();



#endif
