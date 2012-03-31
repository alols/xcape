/************************************************************************
 * xcape.c
 *
 * Copyright 2012 Albin Olsson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>


/************************************************************************
 * Internal data types
 ***********************************************************************/
typedef struct _XCape_t
{
    Display *data_conn;
    Display *ctrl_conn;
    XRecordContext record_ctx;
    KeyCode control_key;
    KeyCode escape_key;
    pthread_t sigwait_thread;
    sigset_t sigset;
    Bool debug;
} XCape_t;


/************************************************************************
 * Internal function declarations
 ***********************************************************************/
void *sig_handler (void *user_data);

void intercept (XPointer user_data, XRecordInterceptData *data);


/************************************************************************
 * Main function
 ***********************************************************************/
int main (int argc, char **argv)
{
    XCape_t *self = malloc (sizeof (XCape_t));
    int dummy;

    if (argc > 1)
    {
        if (0 == strncmp(argv[1], "--debug", 7))
        {
            self->debug = True;
        }
        else
        {
            fprintf (stdout, "Usage: %s [--debug]\n", argv[0]);
            fprintf (stdout,
                         "Runs as a daemon unless --debug flag is set\n");
            return EXIT_SUCCESS;
        }
    }
    else
    {
        self->debug = False;
        daemon (0, 0);
    }

    self->data_conn = XOpenDisplay (NULL);
    self->ctrl_conn = XOpenDisplay (NULL);

    self->escape_key  = XKeysymToKeycode (self->ctrl_conn,
            XK_Escape);
    self->control_key = XKeysymToKeycode (self->ctrl_conn,
            XK_Control_L);

    if (!XQueryExtension (self->ctrl_conn,
                "XTEST", &dummy, &dummy, &dummy))
    {
        fprintf (stderr, "Xtst extension missing\n");
        exit (EXIT_FAILURE);
    }
    if (!XRecordQueryVersion (self->ctrl_conn, &dummy, &dummy))
    {
        fprintf (stderr, "Failed to obtain xrecord version\n");
        exit (EXIT_FAILURE);
    }

    sigfillset(&self->sigset);
    pthread_sigmask (SIG_BLOCK, &self->sigset, NULL);

    pthread_create (&self->sigwait_thread,
            NULL, sig_handler, self);

    XRecordRange *rec_range = XRecordAllocRange();
    rec_range->device_events.first = KeyPress;
    rec_range->device_events.last = KeyRelease;
    XRecordClientSpec client_spec = XRecordAllClients;

    self->record_ctx = XRecordCreateContext (self->ctrl_conn,
            0, &client_spec, 1, &rec_range, 1);

    if (self->record_ctx == 0)
    {
        fprintf (stderr, "Failed to create xrecord context\n");
        exit (EXIT_FAILURE);
    }

    XSync (self->ctrl_conn, False);

    if (!XRecordEnableContext (self->data_conn,
                self->record_ctx, intercept, (XPointer)self))
    {
        fprintf (stderr, "Failed to enable xrecord context\n");
        exit (EXIT_FAILURE);
    }

    if (!XRecordFreeContext (self->ctrl_conn, self->record_ctx))
    {
        fprintf (stderr, "Failed to free xrecord context\n");
    }

    XCloseDisplay (self->ctrl_conn);
    XCloseDisplay (self->data_conn);

    if (self->debug) fprintf (stdout, "main exiting\n");

    return EXIT_SUCCESS;
}


/************************************************************************
 * Internal functions
 ***********************************************************************/
void *sig_handler (void *user_data)
{
    XCape_t *self = (XCape_t*)user_data;
    int sig;

    if (self->debug) fprintf (stdout, "sig_handler running...\n");

    sigwait(&self->sigset, &sig);

    if (self->debug) fprintf (stdout, "Caught signal %d!\n", sig);

    if (!XRecordDisableContext (self->ctrl_conn,
                self->record_ctx))
    {
        fprintf (stderr, "Failed to disable xrecord context\n");
        exit(EXIT_FAILURE);
    }

    XSync (self->ctrl_conn, False);

    if (self->debug) fprintf (stdout, "sig_handler exiting...\n");

    return NULL;
}


void intercept (XPointer user_data, XRecordInterceptData *data)
{
    XCape_t *self = (XCape_t*)user_data;
    static Bool ctrl_pressed = False;
    static Bool ctrl_used    = False;

    if (data->category == XRecordFromServer)
    {
        int     key_event = data->data[0];
        KeyCode key_code  = data->data[1];

        if (self->debug) fprintf (stdout,
                "Intercepted key event %d, key code %d\n",
                key_event, key_code);

        if (key_code == self->control_key)
        {
            if (key_event == KeyPress)
            {
                if (self->debug) fprintf (stdout, "Control pressed!\n");
                ctrl_pressed = True;
            }
            else
            {
                if (self->debug) fprintf (stdout, "Control released!\n");
                if (ctrl_used == False)
                {
                    if (self->debug) fprintf (stdout,
                            "Generating ESC!\n");

                    XTestFakeKeyEvent (self->ctrl_conn,
                            self->escape_key, True, 0);
                    XTestFakeKeyEvent (self->ctrl_conn,
                            self->escape_key, False, 0);
                    XFlush (self->ctrl_conn);
                }
                ctrl_pressed = False;
                ctrl_used = False;
            }
        }
        else if (ctrl_pressed && key_event == KeyPress)
        {
            ctrl_used = True;
        }
    }

    XRecordFreeData (data);
}
