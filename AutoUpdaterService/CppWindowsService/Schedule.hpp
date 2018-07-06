
namespace updater{

enum class UpdateType{
    never,               ///<updater doesn't do anything
    withUserNotification, ///<ask User install update
    silence             ///<install updates without asking User
};
enum class UpdateFreq{
    eachHour,
    eachDay,        ///settings provides time of update
    eachWeek,        ///<settings provides days and time of update
    eachNSecs,
    eachNMins
};

constexpr auto updateDefaultType = UpdateType::withUserNotification;
constexpr auto updateDefaultFreq = UpdateFreq::eachHour;
constexpr auto updateDefaultTime = "12:00";
constexpr auto updateDefaultDays = "0";
constexpr auto updateDefaultNSecs = 15;
constexpr auto updateDefaultNMins = 5;

constexpr auto *updateTypeKey = "updateType";
constexpr auto *UpdateFreqKey = "UpdateFreq";
constexpr auto *UpdateTimeKey = "UpdateTime";
constexpr auto *UpdateTimeInWeekKey = "UpdateTimeInWeek";
constexpr auto *UpdateDaysKey = "UpdateDays";
constexpr auto *UpdateNSecsKey = "UpdateNSecs";
constexpr auto *UpdateNMinsKey = "UpdateNMins";
}
