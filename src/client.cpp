#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cstring> 
#include <iostream> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h> 

std::string getClipboardContent() {
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Unable to open X display\n";
        return "";
    }

    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
    Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    Atom property = XInternAtom(display, "XSEL_DATA", False);

    XConvertSelection(display, clipboard, utf8, property, window, CurrentTime);

    std::string result;
    XEvent event;
    while (true) {
        XNextEvent(display, &event);
        if (event.type == SelectionNotify) {
            if (event.xselection.selection == clipboard) {
                if (event.xselection.property) {
                    Atom actualType;
                    int actualFormat;
                    unsigned long nitems, bytesAfter;
                    unsigned char *data = nullptr;

                    XGetWindowProperty(display, window, property, 0, (~0L), False,
                                       AnyPropertyType, &actualType, &actualFormat,
                                       &nitems, &bytesAfter, &data);

                    if (data) {
                        result = std::string(reinterpret_cast<char*>(data), nitems);
                        XFree(data);
                    }
                }
            }
            break;
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return result;
}


void setClipboardContent(const std::string &text) {
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Unable to open X display\n";
        return;
    }

    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
    Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    Atom targets = XInternAtom(display, "TARGETS", False);

    XSetSelectionOwner(display, clipboard, window, CurrentTime);
    if (XGetSelectionOwner(display, clipboard) != window) {
        std::cerr << "Unable to set selection owner\n";
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        return;
    }

    XEvent event;
    bool running = true;
    while (running) {
        XNextEvent(display, &event);
        if (event.type == SelectionRequest) {
            XSelectionRequestEvent *req = &event.xselectionrequest;
            XSelectionEvent sev;
            memset(&sev, 0, sizeof(sev));
            sev.type = SelectionNotify;
            sev.display = req->display;
            sev.requestor = req->requestor;
            sev.selection = req->selection;
            sev.time = req->time;
            sev.target = req->target;
            sev.property = req->property;

            if (req->target == targets) {
                Atom types[2] = { targets, utf8 };
                XChangeProperty(req->display, req->requestor, req->property, XA_ATOM, 32,
                                PropModeReplace, (unsigned char *)types, 2);
            } else if (req->target == utf8) {
                XChangeProperty(req->display, req->requestor, req->property, utf8, 8,
                                PropModeReplace, (unsigned char *)text.c_str(), text.size());
            } else {
                sev.property = None;
            }

            XSendEvent(display, req->requestor, False, 0, (XEvent *)&sev);
        } else if (event.type == SelectionClear) {
            running = false;
        }
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

const char get[] = "get?";
const char set[] = "set:";

char buffer[1024] = { 0 };

void readFromSocket() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5959);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.0.112");

    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    send(clientSocket, get, strlen(get), 0);

    recv(clientSocket, buffer, sizeof(buffer), 0);

    close(clientSocket);


}


void writeToSocket(const char data[]) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5959);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.0.112");

    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    std::string msg = set;
    msg += data;

    send(clientSocket, msg.c_str(), strlen(msg.c_str()), 0);
    close(clientSocket);
}


std::string clientData = "";
std::string serverData = "";


int main() {
    while (true) {
        std::string data = getClipboardContent();
        std::cout << "get:" << data;
        if(data != clientData) writeToSocket(clientData.c_str()); clientData = data;

        readFromSocket();
        serverData = buffer;
        std::cout << "set:" << serverData;
        if(clientData != serverData) setClipboardContent(serverData); clientData = serverData;
    }
}
