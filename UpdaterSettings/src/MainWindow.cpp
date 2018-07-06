#include "MainWindow.hpp"
#include "Settings.hpp"


#include <QDateTimeEdit>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QSet>
#include <QCheckBox>
#include <QSpinBox>

class MainWindowPrivate{
public:
    MainWindowPrivate()
    {
    }

public:
    QVBoxLayout *mainLayout = nullptr;
    Settings *settings = nullptr;
};


/**
 * @brief construct MDI main window
 *
 * It constructs MainWindow as MDI window and adds 'Add Client' menu item in to the menu bar
 */
MainWindow::MainWindow()
    :QMainWindow(), p(new MainWindowPrivate())
{
    auto central = new QWidget();
    p->mainLayout = new QVBoxLayout(central);
    setCentralWidget(central);
    p->settings = new Settings(Settings::configFilePath("MetaHash"));
    {
    // update type settings
    auto * box = new QGroupBox(tr("Update:"));
    p->mainLayout->addWidget(box);
    auto buton1 = new QRadioButton(tr("Never"));
    auto buton2 = new QRadioButton(tr("Ask before install"));
    auto buton3 = new QRadioButton(tr("Install updates silently"));
    box->setLayout(new QVBoxLayout());
    box->layout()->addWidget(buton1);
    box->layout()->addWidget(buton2);
    box->layout()->addWidget(buton3);

    const auto type = p->settings->updateType();

    buton1->setChecked(type == updater::UpdateType::never);
    buton2->setChecked(type == updater::UpdateType::withUserNotification);
    buton3->setChecked(type == updater::UpdateType::silence);

    connect(buton1, &QRadioButton::toggled, this, [this](bool arg){
        if (arg)
            p->settings->setUpdateType(updater::UpdateType::never);
    });

    connect(buton2, &QRadioButton::toggled, this, [this](bool arg){
        if (arg)
            p->settings->setUpdateType(updater::UpdateType::withUserNotification);
    });

    connect(buton3, &QRadioButton::toggled, this, [this](bool arg){
        if (arg)
            p->settings->setUpdateType(updater::UpdateType::silence);
    });
    }


    {
        //how often to update
        auto * box = new QGroupBox(tr("Update Each:"));
        p->mainLayout->addWidget(box);
        auto buttonEachNSecs = new QRadioButton(tr("Seconds"));
        auto secondsSpinBox = new QSpinBox();
        secondsSpinBox->setMinimum(1);
        secondsSpinBox->setValue(p->settings->UpdateNSecs());
        secondsSpinBox->setEnabled(false);

        auto buttonEachNMins = new QRadioButton(tr("Minutes"));
        auto minutesSpinBox = new QSpinBox();
        minutesSpinBox->setMinimum(1);
        minutesSpinBox->setValue(p->settings->UpdateNMins());
        minutesSpinBox->setEnabled(false);

        auto buton1 = new QRadioButton(tr("Hour"));
        auto buton2 = new QRadioButton(tr("Day"));

        auto timeEdit1 = new QTimeEdit();
        timeEdit1->setDisplayFormat("HH:mm");

        timeEdit1->setTime(p->settings->updateTime());
        timeEdit1->setEnabled(false);

        auto buton3 = new QRadioButton(tr("Week"));
        auto timeEdit2 = new QTimeEdit();
        timeEdit2->setEnabled(false);

        timeEdit2->setDisplayFormat("HH:mm");
        timeEdit2->setTime(p->settings->updateTimeInWeek());

        const auto daysList = p->settings->updateDays();

        auto * daysBox = new QGroupBox(tr("Days:"));
        daysBox->setEnabled(false);
        {
            auto gridLayout = new QGridLayout(daysBox);
            for (int i=0; i<7; ++i){
                const auto& dayOfWeek = i+1;
                auto button = new QCheckBox(QDate::longDayName(dayOfWeek));

                button->setChecked(daysList.contains(i));
                const auto &row = i<4 ? 0 : 1;
                gridLayout->addWidget(button, row, i - row*4 );

                connect(button, &QCheckBox::toggled, this, [this, i](bool arg){
                    auto days = p->settings->updateDays();
                    if (arg)
                        days.insert(i);
                    else
                        days.remove(i);
                    p->settings->setUpdateDays(days);
                });
            }
        }

        box->setLayout(new QVBoxLayout());
        box->layout()->addWidget(buttonEachNSecs);
        box->layout()->addWidget(secondsSpinBox);

        box->layout()->addWidget(buttonEachNMins);
        box->layout()->addWidget(minutesSpinBox);

        box->layout()->addWidget(buton1);

        box->layout()->addWidget(buton2);
        box->layout()->addWidget(timeEdit1);

        box->layout()->addWidget(buton3);
        box->layout()->addWidget(timeEdit2);
        box->layout()->addWidget(daysBox);

        connect(timeEdit1, &QTimeEdit::timeChanged, this, [this](const QTime &time){
            p->settings->setUpdateTime(time);
        });

        connect(timeEdit2, &QTimeEdit::timeChanged, this, [this](const QTime &time){
            p->settings->setUpdateTimeInWeek(time);
        });

        connect(secondsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value){
           p->settings->setUpdateNSecs(value);
        });

        connect(minutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value){
           p->settings->setUpdateNMins(value);
        });

        connect(buton1, &QRadioButton::toggled, this, [this](bool arg){
            if (arg)
                p->settings->setUpdateFreq(updater::UpdateFreq::eachHour);
        });

        connect(buton2, &QRadioButton::toggled, this, [this, timeEdit1](bool arg){
            if (arg)
                p->settings->setUpdateFreq(updater::UpdateFreq::eachDay);
            timeEdit1->setEnabled(arg);
        });

        connect(buton3, &QRadioButton::toggled, this, [this, timeEdit2,daysBox](bool arg){
            if (arg)
                p->settings->setUpdateFreq(updater::UpdateFreq::eachWeek);
            timeEdit2->setEnabled(arg);
            daysBox->setEnabled(arg);
        });

        connect(buttonEachNSecs, &QRadioButton::toggled, this, [this, secondsSpinBox](bool checked){
            if (checked)
                p->settings->setUpdateFreq(updater::UpdateFreq::eachNSecs);
            secondsSpinBox->setEnabled(checked);
            p->settings->setUpdateNSecs(secondsSpinBox->value());
        });

        connect(buttonEachNMins, &QRadioButton::toggled, this, [this, minutesSpinBox](bool checked){
            if (checked)
                p->settings->setUpdateFreq(updater::UpdateFreq::eachNMins);
            minutesSpinBox->setEnabled(checked);
            p->settings->setUpdateNMins(minutesSpinBox->value());
        });

        buton1->setChecked(p->settings->updateFreq() == updater::UpdateFreq::eachHour);
        buton2->setChecked(p->settings->updateFreq() == updater::UpdateFreq::eachDay);
        buton3->setChecked(p->settings->updateFreq() == updater::UpdateFreq::eachWeek);
        buttonEachNSecs->setChecked(p->settings->updateFreq() == updater::UpdateFreq::eachNSecs);
        buttonEachNMins->setChecked(p->settings->updateFreq() == updater::UpdateFreq::eachNMins);
    }

}

MainWindow::~MainWindow()
{

}



