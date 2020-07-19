/*
* Copyright (C) 19aa <Lord Brookie>
* This program is free software. You can redistribute it and/or
* modify it under the terms of the General Public License
* GNU as published by the Free Software Foundation,
* either version 2 of said License or --according to its
* choice-- of any later version.
* This program is distributed in the hope that it will be
* useful, but WITHOUT ANY WARRANTY, even without warranty
* MERCANTIL implicit or without guaranteeing CONVENIENCE FOR A
* PARTICULAR PURPOSE. For more details, see the License
* GNU General Public.
* You should have received a copy of the Public License
* General along with this program. Otherwise, write
* to the Free Software Foundation, Inc., at 675 Mass Ave,
* Cambridge, MA 02139, USA.
*/

#include "save_and_connect.h"

static int
check_wpa_supplicant(void)
{
        int state       = system("which wpa_supplicant > /dev/null 2>&1");
        return state;
}

static int
restart_service(void)
{
        int state       = system("/etc/init.d/wpa_supplicant restart > /dev/null 2>&1");
        return state;
}

gint
save_and_connect(GtkWidget *parent)
{
        GtkMessageType mtype    = GTK_MESSAGE_WARNING;
        GtkButtonsType btype    = GTK_BUTTONS_OK;
        gchar *title, *message;

        if (check_wpa_supplicant()) {
                title           = "Warning";
                message         = "The WPA Supplicant binary has not been found in your system.";
                return show_message(GTK_WINDOW(parent), mtype, btype, title, message);
        }

        const gchar *ssid       = gtk_entry_buffer_get_text(ssid_buffer);
        const gchar *password   = gtk_entry_buffer_get_text(password_buffer);
        const gchar *security   = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(security_combo));
        const gchar *wpa_file   = "/etc/wpa_supplicant/wpa_supplicant.conf";
        gchar scan_ssid;
        gboolean is_there_an_error = FALSE;

        if (security == NULL) {
                title           = "Unselected protocol";
                message         = "The security protocol has not been chosen.";
                is_there_an_error = TRUE;
        } else if (strlen(password) < 8 && strncmp(security, "Open", 4)) {
                title           = "Short password";
                message         = "The given password is so short.";
                is_there_an_error = TRUE;
        }

        if (is_there_an_error) {
                return show_message(GTK_WINDOW(parent), mtype, btype, title, message);
        }

        scan_ssid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hidden_check)) ? '1' : '0';

        FILE *fp;
        gchar *hashed_pass;

        if (strncmp(security, "Open", 4)) {
                size_t hashed_pass_len  = 80;
                size_t command_len      = 16 + strlen(ssid) + 2 + strlen(password) + 26;

                hashed_pass      = (gchar *) malloc(hashed_pass_len);
                gchar *command   = (gchar *) malloc(command_len + 1);

                snprintf(command, command_len, "wpa_passphrase \"%s\" \"%s\" | grep psk | tail -n 1", ssid, password);
                fp                = popen(command, "r");

                hashed_pass     = fgets(hashed_pass, hashed_pass_len, fp);
                hashed_pass[hashed_pass_len - 2] = '\0'; /* Deleting the '\n' character. */

                free(command);
                pclose(fp);
        }

        size_t content_len = 254;
        gchar *content         = (gchar *) malloc(content_len + 1);

        if (strncmp(security, "WPA2-PSK", 7) == 0) {

                snprintf(content, 1 + content_len, "\nnetwork={\n\
\tssid=\"%s\"\n\
\tscan_ssid=%c\n\
%s\
\tpriority=5\n\
}", ssid, scan_ssid, hashed_pass);

        free(hashed_pass);

        /* endif */
        } else {

                snprintf(content, 1 + content_len, "\nnetwork={\n\
\tssid=\"%s\"\n\
\tscan_ssid=%c\n\
\tkey_mgmt=NONE\n\
\tpriority=5\n\
}", ssid, scan_ssid);

        /* end else */
        }

        fp              = fopen(wpa_file, "a");
        fwrite(content, sizeof(gchar), strlen(content), fp);

        fclose(fp);
        free(content);

        mtype   = GTK_MESSAGE_QUESTION;
        btype   = GTK_BUTTONS_YES_NO;
        title   = "Congratulation";
        message = "The network has been added successfully!\n\
Would you like to restart the WPA Supplicant service?";

        gint response   = show_message(GTK_WINDOW(parent), mtype, btype, title, message);
        if (response == GTK_RESPONSE_YES) {
                btype   = GTK_BUTTONS_OK;
                int state       = restart_service();

                if (state == 0) {
                        mtype   = GTK_MESSAGE_INFO;
                        message = "The WPA Supplicant service has been restarted successfully.";
                } else {
                        mtype   = GTK_MESSAGE_WARNING;
                        message = "An error occurred while the WPA Supplicant\n\
service was being restarted.";
                }

                show_message(GTK_WINDOW(parent), mtype, btype, title, message);
        }

        gtk_entry_buffer_set_text(ssid_buffer, "", -1);
        gtk_entry_buffer_set_text(password_buffer, "", -1);
        gtk_combo_box_set_active(GTK_COMBO_BOX(security_combo), -1);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hidden_check), FALSE);

        return 0;
}
