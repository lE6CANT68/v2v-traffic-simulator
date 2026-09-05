QT += core gui widgets openglwidgets network
LIBS += -lopengl32
CONFIG += c++17

SOURCES += \
    Graphe.cpp \
    Grapheinterference.cpp \
    Mainwindow.cpp \
    OpenGLWidget.cpp \
    Voiture.cpp \
    main.cpp \
    tinyxml2.cpp

HEADERS += \
    Graphe.h \
    Grapheinterference.h \
    Mainwindow.h \
    Noeud.h \
    OpenGLWidget.h \
    Voiture.h \
    tinyxml2.h

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ressources.qrc
