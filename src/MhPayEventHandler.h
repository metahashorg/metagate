#ifndef MHPAYEVENTHANDLER_H
#define MHPAYEVENTHANDLER_H

#include <QObject>
#include <QWidget>
#include <QFileOpenEvent>
#include <Log.h>

class MhPayEventHandler: public QObject {
  Q_OBJECT
public:
  MhPayEventHandler() {}
  ~MhPayEventHandler() {
  }
protected:
  bool eventFilter(QObject* object, QEvent* event) {
      if (event->type() == QEvent::FileOpen) {
          auto *openEvent = static_cast<QFileOpenEvent *>(event);
          LOG << "Open file" << openEvent->url().toString();
      }
      return false;
  }
};

#endif // MHPAYEVENTHANDLER_H
