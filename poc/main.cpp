﻿#include <QImage>
#include <QRect>
#include <QPainter>
#include <QGuiApplication>
#include <cstdio>
#include <QPaintEngine>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "msgq.cpp"

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

#include <dlfcn.h>
#include <stdint.h>

using namespace std;

//todo: make it singleton
class FBStuff {
    public:
        FBStuff(){
            qputenv("QMLSCENE_DEVICE", "epaper");
            qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");

            //needed for qpainter
            char *argv[0];
            int argc = 0;
            app = new QGuiApplication(argc,argv);
            instance = getInstance();
        }

        void DrawText(int i, char *text, bool wait) {
              QRect rect(100,i,200,100);
              QPainter painter((QPaintDevice*)(instance+8));
              printf("after ctor\n");
              painter.drawText(rect, 132,text);
              painter.end();
              printf("Sending update\n");
              int w = wait ? 3 : 0 ;
              sendUpdate(0, rect, 3,w);
        }


    private:
        uint32_t* instance;
        QGuiApplication *app;

        //black magic
        uint32_t* (*getInstance)(void) = (uint32_t*(*)(void)) 0x224BC;
        void (*sendUpdate)(void*, ...) = (void (*)(void*,...)) 0x2257C;
};


extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

int main(int argc, char **argv, char **envp) {
  FBStuff stuff;
  for(int i = 0; i < 1000; i+=30){
      stuff.DrawText(i, "Testing", false);
  }
  stuff.DrawText(1800, "Done", true);

  while (true) {
    ipc::msgbuf buf = MSGQ.recv();
  }
  printf("END of our main\n");
}

int __libc_start_main(int (*_main)(int, char **, char **), int argc,
                      char **argv, int (*init)(int, char **, char **),
                      void (*fini)(void), void (*rtld_fini)(void),
                      void *stack_end) {

  printf("LIBC START HOOK\n");

  typeof(&__libc_start_main) func_main =
      (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");

  return func_main(main, argc, argv, init, fini, rtld_fini, stack_end);
};
};
