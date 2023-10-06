#include "../../../gtk_functions.h"
#include "../../../bt_functions.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h> // Added for memory allocation
#include <string.h>
#include <cjson/cJSON.h>


typedef struct {
    char name[128];
} UserSettings;



//SAVE USER TO JSON
int saveUser(const char *username) {
    //User struct
    UserSettings *user = (UserSettings *)malloc(sizeof(UserSettings));
    if (user == NULL) return -1;
    strcpy(user->name, username);

    // READ EXISTING FILE
    const char *filePath = "user.json";
    FILE *file = fopen(filePath, "r");
    cJSON *root = NULL;
    
    // IF FILE EXIST PARSE TO JSON
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *fileContent = (char *)malloc(fileSize + 1);
        if (fileContent == NULL) {
            fclose(file);
            free(user);
            return -1;
        }

        fread(fileContent, 1, fileSize, file);
        fileContent[fileSize] = '\0';
        fclose(file);

        root = cJSON_Parse(fileContent);
        free(fileContent);
    }
    //IF DOESNT EXIST OR IS EMPTY NEW
    if (root == NULL) {root = cJSON_CreateArray();}

    // IF NOT EMPTY CHECK 
    else {
        // Check if the name is already in the JSON array
        int nameExists = 0;
        cJSON *userItem;
        cJSON_ArrayForEach(userItem, root) {
            cJSON *usernameItem = cJSON_GetObjectItemCaseSensitive(userItem, "username");
            if (cJSON_IsString(usernameItem) && (strcmp(usernameItem->valuestring, user->name) == 0)) {
                nameExists = 1;
                break;
            }
        }
        if (nameExists) {
            printf("User with the same name already exists in the JSON.\n");
            cJSON_Delete(root);
            free(user);
            return 3; //ERROR CODE FOR EXISTING NAME
        }
        return 0;
    }

    // JSON OBJ FOR USR
    cJSON *userObject = cJSON_CreateObject();
    cJSON_AddStringToObject(userObject, "username", user->name);
    cJSON_AddItemToArray(root, userObject);

    //JSON ARR TO STR
    char *jsonString = cJSON_Print(root);

    // JSON DATA TO FILE
    file = fopen(filePath, "w");
    if (file != NULL) {
        fputs(jsonString, file);
        fclose(file);
        printf("User settings saved to %s\n", filePath);
    } else {
        printf("Error: Unable to save user settings to %s\n", filePath);
    }

    // CLEAN
    cJSON_Delete(root);
    free(jsonString);
    free(user);

    return 0;
}

//LOADS THE JSON 
cJSON *loadUser() {
    const char *filePath = "user.json";
    FILE *file = fopen(filePath, "r");
    cJSON *root = NULL;

    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *fileContent = (char *)malloc(fileSize + 1);
        if (fileContent == NULL) {
            fclose(file);
            return NULL;
        }

        fread(fileContent, 1, fileSize, file);
        fileContent[fileSize] = '\0';
        fclose(file);

        root = cJSON_Parse(fileContent);
        free(fileContent);
    }

    return root;
}