#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "qrcoder.h"

const std::string text = std::string("rddomdfmifdimkgmkglkmgflkmgfmlkgfmlkgmlkmklgkmlgklmgdkldmmklgfmklgflkmfklmfgkmlgfklmgktrlkmkldfmkdsmmdsfmdf,.mfdmgfkm"
                       "\x01\x0A"
                       "mfcgnmgfkjglkmflkmrlkmrklmfmklgfmkllkmg  gf klfc mf lmmglkmg "
                       "gl kg lg lg lh klhl;kmfdlkfdl5io059dsi9d09ifjfivciovoivovcopcv"
                       "\x13ttrjkjgfkjgfkjgfkgfkjgfgf"
                       "gjkjgjkgkjgkjgfjkgfjkgfjkgkjgjkgkjgjkkjgf") + std::string(10, '\0');

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug() << text.size();
    QByteArray data = QByteArray::fromStdString(text);
    qDebug() << "IN:" << data;
    QByteArray bin = QRCoder::encode(data);
    QPixmap pix;
    pix.loadFromData(bin, "PNG");
    bin = bin.toBase64();
    QByteArray res = QRCoder::decode(QByteArray::fromBase64(bin));
    qDebug() << "OUT:" << res;
    qDebug() << "EQUAL?" << (res == data);
    ui->label->setPixmap(pix);
    pix.save("out.png");
}

MainWindow::~MainWindow()
{
    delete ui;
}
