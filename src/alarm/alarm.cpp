#include "alarm/alarm.h"
#include <chrono>

Alarm::Alarm() {}
Alarm::Alarm(int target_channel, std::string des, std::string cond, int r_lv)
{
    this->target_channel = target_channel;
    this->name = des;
    this->condition = cond;
    this->alarm_id = r_lv;
}
Alarm::~Alarm() {}
void Alarm::set_target_channel(int target_channel)
{
    this->target_channel = target_channel;
}
void Alarm::set_name(std::string des)
{
    this->name = des;
}
void Alarm::set_condition(std::string cond)
{
    this->condition = cond;
}
void Alarm::set_alarm_id(int r_lv)
{
    this->alarm_id = r_lv;
}
void Alarm::set_alarm_sentence(std::string s)
{
    this->alarm_sentence = s;
}
void Alarm::set_alarm_context(std::unordered_map<int, std::string> context)
{
    this->alarm_context = context;
}

// 1106 hj modbus 적용
void Alarm::set_id(int id)
{
    this->id = id;
}

int Alarm::get_target_channel()
{
    return this->target_channel;
}
std::string Alarm::get_name()
{
    return this->name;
}
std::string Alarm::get_condition()
{
    return this->condition;
}
int Alarm::get_alarm_id()
{
    return this->alarm_id;
}
std::string Alarm::get_alarm_sentence()
{
    return this->alarm_sentence;
}
// 1106 hj modbus 적용
int Alarm::get_id()
{
    return this->id;
}

bool define_alarm(std::string condition, const std::deque<detected_history_item> &detected_classes_history, double persist_seconds)
{
    if (condition.empty())
    {
        return false;
    }
    std::vector<bool> history_result;
    for (auto item : detected_classes_history)
    {
        std::unordered_set<int> classElement(item.detected_classes.begin(), item.detected_classes.end());
        std::stringstream ss(condition);
        std::stack<bool> value_stack;
        std::string token;
        bool isNot = false;
        while (ss >> token)
        {
            if (isdigit(token[0]))
            {
                bool t = (classElement.count(std::stoi(token)) != 0);
                if (isNot)
                {
                    t = !t;
                }
                value_stack.push(t);
                isNot = false;
            }
            else if (token == "and")
            {
                bool val2 = value_stack.top();
                value_stack.pop();
                bool val1 = value_stack.top();
                value_stack.pop();

                value_stack.push(val1 && val2);
            }
            else if (token == "or")
            {
                bool val2 = value_stack.top();
                value_stack.pop();
                bool val1 = value_stack.top();
                value_stack.pop();

                value_stack.push(val1 || val2);
            }
            else if (token == "not")
            {
                isNot = true;
            }
        }
        history_result.push_back(value_stack.top());
    }
    bool res = true;

    int size = history_result.size();
    if (size < frames_to_check)
        return false;

    for (int i = size - frames_to_check; i < size; i++)
    {
        if (!history_result[i])
        {
            res = false;
            break;
        }
    }

    return res;
}