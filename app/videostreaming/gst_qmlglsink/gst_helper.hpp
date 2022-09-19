#if defined(ENABLE_GSTREAMER)
#ifndef GST_PLATFORM_INCLUDE_H
#define GST_PLATFORM_INCLUDE_H


// Use this file to include gstreamer into your project, independend of platform
// (TODO: rn everythng except standart ubuntu has been deleted).
// exposes a initGstreamerOrThrow() method that should be called before any actual gstreamer calls.

#include "../QOpenHDVideoHelper.hpp"
#include "qglobal.h"
#include <gst/gst.h>
#include <QString>
#include <qquickitem.h>
#include <stdexcept>
#include <sstream>
#include <qqmlapplicationengine.h>

/**
 * @brief initGstreamer, throw a run time exception when failing
 */
static void initGstreamerOrThrow(){
    GError* error = nullptr;
    if (!gst_init_check(nullptr,nullptr, &error)) {
        std::stringstream ss;
        ss<<"Cannot initialize gstreamer";
        ss<<error->code<<":"<<error->message;
        g_error_free(error);
        throw std::runtime_error(ss.str().c_str());
    }
}

// Similar to above, but takes argc/ argv from command line.
// This way it is possible to pass on extra stuff at run time onto gstreamer by launching
// QOpenHD with some argc/ argvalues
static void initGstreamerOrThrowExtra(int argc,char* argv[]){
    GError* error = nullptr;
    if (!gst_init_check(&argc,&argv, &error)) {
        std::stringstream ss;
        ss<<"Cannot initialize gstreamer";
        ss<<error->code<<":"<<error->message;
        g_error_free(error);
        throw std::runtime_error(ss.str().c_str());
    }
}

// If qmlgl plugin was dynamically linked, this will force GStreamer to go find it and
// load it before the QML gets loaded in main.cpp (without this, Qt will complain that
// it can't find org.freedesktop.gstreamer.GLVideoItem)
static void initQmlGlSinkOrThrow(){
    /*if (!gst_element_register (plugin, "qmlglsink",
              GST_RANK_NONE, GST_TYPE_QT_SINK)) {
         qDebug()<<"Cannot iregister gst qmlglsink";
      }*/
    GstElement *sink = gst_element_factory_make("qmlglsink", NULL);
    if(sink==nullptr){
        qDebug()<<"Cannot initialize gstreamer - qmlsink not found";
        //throw std::runtime_error("Cannot initialize gstreamer - qmlsink not found\n");
   }
}

// not sure, customize the path where gstreamer log is written to
static void customizeGstreamerLogPath(){
    char debuglevel[] = "*:3";
    #if defined(__android__)
    char logpath[] = "/sdcard";
    #else
    char logpath[] = "/tmp";
    #endif
    qputenv("GST_DEBUG", debuglevel);
    QString file = QString("%1/%2").arg(logpath).arg("gstreamer-log.txt");
    qputenv("GST_DEBUG_NO_COLOR", "1");
    qputenv("GST_DEBUG_FILE", file.toStdString().c_str());
    qputenv("GST_DEBUG_DUMP_DOT_DIR", logpath);
}

static QString get_gstreamer_version() {
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    QString gst_ver = QString();
    QTextStream s(&gst_ver);
    s << major;
    s << ".";
    s << minor;
    s << ".";
    s << micro;
    return gst_ver;
}

// link gstreamer qmlglsink to qt window
static void link_gsteamer_to_qt_window(GstElement *qmlglsink,QQuickItem *qtOutWindow){
      g_object_set(qmlglsink, "widget", qtOutWindow, NULL);
}

// find qmlglsink in gstreamer pipeline and link it to the window
static void link_gstreamer_pipe_to_qt_window(GstElement * m_pipeline,QQuickItem *qtOutWindow){
    GstElement *qmlglsink = gst_bin_get_by_name(GST_BIN(m_pipeline), "qmlglsink");
    if(!qmlglsink){
        qDebug()<<"link_gstreamer_pipe_to_qt_window: no qmlglimagesink";
        return;
    }
    assert(qmlglsink!=nullptr);
    link_gsteamer_to_qt_window(qmlglsink,qtOutWindow);
}

/**
 * Find the qt window where video is output to.
 * @param isMainStream QOpenHD supports upt to 2 simultaneous streams, they each have their own respective window.
 * @return nullptr if window cannot be found, the window to display video with otherwise.
 */
static QQuickItem* find_qt_video_window(QQmlApplicationEngine& m_engine,const bool isMainStream=true){
    QString m_elementName;
    if(isMainStream){
         m_elementName = "mainVideoGStreamer";
    }else{
         m_elementName = "pipVideoGStreamer";
    }
    QQuickItem *videoItem;
    QQuickWindow *rootObject;
    auto rootObjects = m_engine.rootObjects();
    if (rootObjects.length() < 1) {
        qDebug() << "Failed to obtain root object list!";
        return nullptr;
    }
    rootObject = static_cast<QQuickWindow *>(rootObjects.first());
    videoItem = rootObject->findChild<QQuickItem *>(m_elementName.toUtf8());
    if (videoItem == nullptr) {
        qDebug() << "Failed to obtain video item pointer for " << m_elementName;
        return nullptr;
    }
    qDebug()<<"Found (qmlglsink) called"<<m_elementName;
    return videoItem;
}

// Creates a pipeline whose last element produces rtp h264,h265 or mjpeg data
static std::string create_debug_encoded_data_producer(const QOpenHDVideoHelper::VideoCodec& videoCodec){
    std::stringstream ss;
    ss<<"videotestsrc ! video/x-raw, format=I420,width=640,height=480,framerate=30/1 ! ";
    if(videoCodec==QOpenHDVideoHelper::VideoCodecH264){
        ss<<"x264enc bitrate=5000 tune=zerolatency key-int-max=10 ! h264parse config-interval=-1 ! ";
        ss<<"rtph264pay mtu=1024 ! ";
    }else if(videoCodec==QOpenHDVideoHelper::VideoCodecH265){
        ss<<"x265enc bitrate=5000 tune=zerolatency ! ";
        ss<<"rtph265pay mtu=1024 ! ";
    }else{
        ss<<"jpegenc ! ";
        ss << "rtpjpegpay mtu=1024 ! ";
    }
    ss<<"queue ! ";
    return ss.str();
}
#endif // GST_PLATFORM_INCLUDE_H
#endif
