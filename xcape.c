/************************************************************************
 * xcape.c
 *
 * Copyright 2015 Albin Olsson
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
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>


/************************************************************************
 * Internal data types
 ***********************************************************************/
typedef struct _Key_t
{
    KeyCode key;
    struct _Key_t *next;
} Key_t;

typedef struct _KeyMap_t
{
    Bool UseKeyCode;        /* (for from) instead of KeySym; ignore latter */
    KeySym from_ks;
    KeyCode from_kc;
    Key_t *to_keys;
    Bool used;
    Bool pressed;
    Bool mouse;
    struct timeval down_at;
    struct _KeyMap_t *next;
} KeyMap_t;

typedef struct _XCape_t
{
    Display *data_conn;
    Display *ctrl_conn;
    XRecordContext record_ctx;
    pthread_t sigwait_thread;
    sigset_t sigset;
    Bool foreground;
    Bool debug;
    KeyMap_t *map;
    Key_t *generated;
    struct timeval timeout;
} XCape_t;

/************************************************************************
 * Internal function declarations
 ***********************************************************************/
void *sig_handler (void *user_data);

void intercept (XPointer user_data, XRecordInterceptData *data);

KeyMap_t *parse_mapping (Display *ctrl_conn, char *mapping, Bool debug);

void delete_mapping (KeyMap_t *map);

Key_t *key_add_key (Key_t *keys, KeyCode key);

void delete_keys (Key_t *keys);

void print_usage (const char *program_name);

/************************************************************************
 * Main function
 ***********************************************************************/
int main (int argc, char **argv)
{
    XCape_t *self = malloc (sizeof (XCape_t));

    int dummy, ch;

    static char default_mapping[] = "Control_L=Escape";
    char *mapping = default_mapping;

    XRecordRange *rec_range = XRecordAllocRange();
    XRecordClientSpec client_spec = XRecordAllClients;

    self->foreground = False;
    self->debug = False;
    self->timeout.tv_sec = 0;
    self->timeout.tv_usec = 500000;
    self->generated = NULL;

    rec_range->device_events.first = KeyPress;
    rec_range->device_events.last = ButtonRelease;

    while ((ch = getopt (argc, argv, "dfe:t:")) != -1)
    {
        switch (ch)
        {
        case 'd':
            self->debug = True;
            /* imply -f (no break) */
        case 'f':
            self->foreground = True;
            break;
        case 'e':
            mapping = optarg;
            break;
        case 't':
            {
                int ms = atoi (optarg);
                if (ms > 0)
                {
                    self->timeout.tv_sec = ms / 1000;
                    self->timeout.tv_usec = (ms % 1000) * 1000;
                }
                else
                {
                    fprintf (stderr, "Invalid argument for '-t': %s.\n", optarg);
                    print_usage (argv[0]);
                    return EXIT_FAILURE;
                }
            }
            break;
        default:
            print_usage (argv[0]);
            return EXIT_SUCCESS;
        }
    }

    if (optind < argc)
    {
        fprintf (stderr, "Not a command line option: '%s'\n", argv[optind]);
        print_usage (argv[0]);
        return EXIT_SUCCESS;
    }

    if (!XInitThreads ())
    {
        fprintf (stderr, "Failed to initialize threads.\n");
        exit (EXIT_FAILURE);
    }

    self->data_conn = XOpenDisplay (NULL);
    self->ctrl_conn = XOpenDisplay (NULL);

    if (!self->data_conn || !self->ctrl_conn)
    {
        fprintf (stderr, "Unable to connect to X11 display. Is $DISPLAY set?\n");
        exit (EXIT_FAILURE);
    }
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
    if (!XkbQueryExtension (self->ctrl_conn, &dummy, &dummy,
            &dummy, &dummy, &dummy))
    {
        fprintf (stderr, "Failed to obtain xkb version\n");
        exit (EXIT_FAILURE);
    }

    self->map = parse_mapping (self->ctrl_conn, mapping, self->debug);

    if (self->map == NULL)
    {
        fprintf (stderr, "Failed to parse_mapping\n");
        exit (EXIT_FAILURE);
    }

    if (self->foreground != True)
        daemon (0, 0);

    sigemptyset (&self->sigset);
    sigaddset (&self->sigset, SIGINT);
    sigaddset (&self->sigset, SIGTERM);
    pthread_sigmask (SIG_BLOCK, &self->sigset, NULL);

    pthread_create (&self->sigwait_thread,
            NULL, sig_handler, self);

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

    pthread_join (self->sigwait_thread, NULL);

    if (!XRecordFreeContext (self->ctrl_conn, self->record_ctx))
    {
        fprintf (stderr, "Failed to free xrecord context\n");
    }

    if (self->debug) fprintf (stdout, "main exiting\n");

    XFree (rec_range);

    XCloseDisplay (self->ctrl_conn);
    XCloseDisplay (self->data_conn);

    delete_mapping (self->map);

    free (self);

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

    XLockDisplay (self->ctrl_conn);

    if (!XRecordDisableContext (self->ctrl_conn,
                self->record_ctx))
    {
        fprintf (stderr, "Failed to disable xrecord context\n");
        exit(EXIT_FAILURE);
    }

    XSync (self->ctrl_conn, False);

    XUnlockDisplay (self->ctrl_conn);

    if (self->debug) fprintf (stdout, "sig_handler exiting...\n");

    return NULL;
}

Key_t *key_add_key (Key_t *keys, KeyCode key)
{
    Key_t *rval = keys;

    if (keys == NULL)
    {
        keys = malloc (sizeof (Key_t));
        rval = keys;
    }
    else
    {
        while (keys->next != NULL) keys = keys->next;
        keys = (keys->next = malloc (sizeof (Key_t)));
    }

    keys->key = key;
    keys->next = NULL;

    return rval;
}

void handle_key (XCape_t *self, KeyMap_t *key,
        Bool mouse_pressed, int key_event)
{
    Key_t *k;

    if (key_event == KeyPress)
    {
        if (self->debug) fprintf (stdout, "Key pressed!\n");

        key->pressed = True;

        gettimeofday (&key->down_at, NULL);

        if (mouse_pressed)
        {
            key->used = True;
        }
    }
    else
    {
        if (self->debug) fprintf (stdout, "Key released!\n");
        if (key->used == False)
        {
            struct timeval timev = self->timeout;
            gettimeofday (&timev, NULL);
            timersub (&timev, &key->down_at, &timev);

            if (timercmp (&timev, &self->timeout, <))
            {
                for (k = key->to_keys; k != NULL; k = k->next)
                {
                    if (self->debug) fprintf (stdout, "Generating %s!\n",
                            XKeysymToString (XkbKeycodeToKeysym (self->ctrl_conn,
                                    k->key, 0, 0)));

                    XTestFakeKeyEvent (self->ctrl_conn,
                            k->key, True, 0);
                    self->generated = key_add_key (self->generated, k->key);
                }
                for (k = key->to_keys; k != NULL; k = k->next)
                {
                    XTestFakeKeyEvent (self->ctrl_conn,
                            k->key, False, 0);
                    self->generated = key_add_key (self->generated, k->key);
                }
                XFlush (self->ctrl_conn);
            }
        }
        key->used = False;
        key->pressed = False;
    }
}

void intercept (XPointer user_data, XRecordInterceptData *data)
{
    XCape_t *self = (XCape_t*)user_data;
    static Bool mouse_pressed = False;
    KeyMap_t *km;

    XLockDisplay (self->ctrl_conn);

    if (data->category == XRecordFromServer)
    {
        int     key_event = data->data[0];
        KeyCode key_code  = data->data[1];
        Key_t *g, *g_prev = NULL;

        for (g = self->generated; g != NULL; g = g->next)
        {
            if (g->key == key_code)
            {
                if (self->debug) fprintf (stdout,
                        "Ignoring generated event.\n");
                if (g_prev != NULL)
                {
                    g_prev->next = g->next;
                }
                else
                {
                    self->generated = g->next;
                }
                free (g);
                goto exit;
            }
            g_prev = g;
        }

        if (self->debug) fprintf (stdout,
                "Intercepted key event %d, key code %d\n",
                key_event, key_code);

        if (key_event == ButtonPress)
        {
            mouse_pressed = True;
        }
        else if (key_event == ButtonRelease)
        {
            mouse_pressed = False;
        }
        for (km = self->map; km != NULL; km = km->next)
        {
            if ((km->UseKeyCode == False
                    && XkbKeycodeToKeysym (self->ctrl_conn, key_code, 0, 0)
                        == km->from_ks)
                || (km->UseKeyCode == True
                    && key_code == km->from_kc))
            {
                handle_key (self, km, mouse_pressed, key_event);
            }
            else if (km->pressed
                    && (key_event == KeyPress || key_event == ButtonPress))
            {
                km->used = True;
            }
        }
    }

exit:
    XUnlockDisplay (self->ctrl_conn); 
    XRecordFreeData (data);
}

KeyMap_t *parse_token (Display *dpy, char *token, Bool debug)
{
    KeyMap_t *km = NULL;
    KeySym    ks;
    char      *from, *to, *key;
    KeyCode   code;           /* keycode */
    long      parsed_code;    /* parsed keycode value */

    to = token;
    from = strsep (&to, "=");
    if (to != NULL)
    {
        km = calloc (1, sizeof (KeyMap_t));

        if (!strncmp (from, "#", 1)
               && strsep (&from, "#") != NULL)
        {
            errno = 0;
            parsed_code = strtoul (from, NULL, 0); /* dec, oct, hex automatically */
            if (errno == 0
                   && parsed_code <=255
                   && XkbKeycodeToKeysym (dpy, (KeyCode) parsed_code, 0, 0) != NoSymbol)
            {
                km->UseKeyCode = True;
                km->from_kc = (KeyCode) parsed_code;
                if (debug)
                {
                  KeySym ks_temp = XkbKeycodeToKeysym (dpy, (KeyCode) parsed_code, 0, 0);
                  fprintf(stderr, "Assigned mapping from \"%s\" ( keysym 0x%x, "
                          "key code %d)\n",
                          XKeysymToString(ks_temp),
                          (unsigned) ks_temp,
                          (unsigned) km->from_kc);
                }
            }
            else
            {
                fprintf (stderr, "Invalid keycode: %s\n", from);
                return NULL;
            }
        }
        else
        {
            if ((ks = XStringToKeysym (from)) == NoSymbol)
            {
                fprintf (stderr, "Invalid key: %s\n", token);
                return NULL;
            }

            km->UseKeyCode  = False;
            km->from_ks     = ks;
            km->to_keys     = NULL;

            if (debug)
            {
              fprintf(stderr, "Assigned mapping from \"%s\" ( keysym 0x%x, "
                      "key code %d)\n",
                      XKeysymToString (km->from_ks),
                      (unsigned) km->from_ks,
                      (unsigned) XKeysymToKeycode (dpy, km->from_ks));
            }
        }

        for(;;)
        {
            key = strsep (&to, "|");
            if (key == NULL)
                break;

            if (!strncmp (key, "#", 1)
                   && strsep (&key, "#") != NULL)
            {
                errno = 0;
                parsed_code = strtoul (key, NULL, 0); /* dec, oct, hex automatically */
                if (!(errno == 0
                      && parsed_code <=255
                      && XkbKeycodeToKeysym (dpy, (KeyCode) parsed_code, 0, 0) != NoSymbol))
                {
                    fprintf (stderr, "Invalid keycode: %s\n", key);
                    return NULL;
                }

                code = (KeyCode) parsed_code;
            }
            else
            {
                if ((ks = XStringToKeysym (key)) == NoSymbol)
                {
                    fprintf (stderr, "Invalid key: %s\n", key);
                    return NULL;
                }

                code = XKeysymToKeycode (dpy, ks);
                if (code == 0)
                {
                    fprintf (stderr, "WARNING: No keycode found for keysym "
                            "%s (0x%x) in mapping %s. Ignoring this "
                            "mapping.\n", key, (unsigned int)ks, token);
                    return NULL;
                }
            }

            km->to_keys = key_add_key (km->to_keys, code);
            if (debug)
            {
              KeySym ks_temp = XkbKeycodeToKeysym (dpy, code, 0, 0);
              fprintf(stderr, "to \"%s\" (keysym 0x%x, key code %d)\n",
                  XKeysymToString(ks_temp),
                  (unsigned) ks_temp,
                  (unsigned) code);
            }
        }
    }
    else
        fprintf (stderr, "WARNING: Mapping without = has no effect: '%s'\n", token);


    return km;
}

KeyMap_t *parse_mapping (Display *ctrl_conn, char *mapping, Bool debug)
{
    char     *token;
    KeyMap_t *rval, *km, *nkm;

    rval = km = NULL;

    for(;;)
    {
        token = strsep (&mapping, ";");
        if (token == NULL)
            break;

        nkm = parse_token (ctrl_conn, token, debug);

        if (nkm != NULL)
        {
            if (km == NULL)
                rval = km = nkm;
            else
            {
                km->next = nkm;
                km = nkm;
            }
        }
    }

    return rval;
}

void delete_mapping (KeyMap_t *map)
{
    while (map != NULL) {
        KeyMap_t *next = map->next;
        delete_keys (map->to_keys);
        free (map);
        map = next;
    }
}

void delete_keys (Key_t *keys)
{
    while (keys != NULL) {
        Key_t *next = keys->next;
        free (keys);
        keys = next;
    }
}

void print_usage (const char *program_name)
{
    fprintf (stdout, "Usage: %s [-d] [-f] [-t timeout_ms] [-e <mapping>]\n", program_name);
    fprintf (stdout, "Runs as a daemon unless -d or -f flag is set\n");
}
