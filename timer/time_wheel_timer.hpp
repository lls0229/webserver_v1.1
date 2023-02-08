#ifndef _LIB_SRC_TIME_WHEEL_TIMER_H_
#define _LIB_SRC_TIME_WHEEL_TIMER_H_

#include"timer_common.hpp"
#include<array>
#include<list>
#include<iostream>

/**
 * @auther: lls
 * @data: 2023-01-24
 * @last modified by: author
 * description: Time Wheel Timer
 */

template <typename _UData>
class TimerNode{
public:
	TimerNode() = default;
	~TimerNode() = default;

public:
	void setTimeSlot(int slot)
	{
		this->_time_slot = slot;
	}

	int getTimeSlot()
	{
		return this->_time_slot;
	}

	void setRotation(int rotation)
	{
		this->_rotation = rotation;
	}

	int getRotation()
	{
		return this->_rotation;
	}
	
public:
	Timer<_UData> timer;

private:
	int _time_slot;  // 记录定时器在时间轮中的槽位
	int _rotation;  // 记录定时器在时间轮转多少圈后生效
};


template <typename _UData>
class TWTimerContainer : public ITimerContainer<_UData>{
public:
	TWTimerContainer();
	~TWTimerContainer() override;

public:
	void tick() override;
	Timer<_UData> *addTimer(time_t timeout) override;
	void delTimer(Timer<_UData> *timer) override;
	void resetTimer(Timer<_UData> *timer, time_t timeout) override;
	int getMinExpire() override;
	
private:
	TimerNode<_UData> * del_timer(Timer<_UData> *timer);
	void add_timer(TimerNode<_UData> *timer_node, time_t timeout);

private:
	// 时间轮上槽的书目为600 转一圈的时间时60s
	static const int _SLOTS_NUM = 600;

	// 时间轮
	using TimerList = std::list<TimerNode<_UData> *>;
	std::array<TimerList *, _SLOTS_NUM> _slots;

	// 时间轮的槽间隔 100ms
	static const int _SI = 100;

	// 时间轮当前的槽
	int _cur_slot;
};

template <typename _UData>
TWTimerContainer<_UData>::TWTimerContainer() : _cur_slot(1)
{
	_slots.fill(nullptr);
}

template <typename _UData>
TWTimerContainer<_UData>::~TWTimerContainer()
{
	TimerList *temp = nullptr;

	for(int i = 0; i < _slots.size(); ++i){
		temp = _slots[i];
		if(temp != nullptr){
			for( auto itr = temp->begin(); itr != temp->end(); ++itr){
				delete *itr;
			}
		}
		delete temp;
	}
}

template <typename _UData>
void TWTimerContainer<_UData>::tick(){
	// 取出当前指针指向的slot中保存的链表
	auto slot_list = _slots[_cur_slot];
	TimerNode<_UData> *node = nullptr;
	if(slot_list){
		for( auto itr = slot_list->begin(); itr != slot_list->end(); ++itr){
			// 如果定时器的rotation大于0， 则它在这一轮不起作用
			if((*itr)->getRotation() > 0){
				(*itr)->setRotation((*itr)->getRotation() - 1);
				continue;
			}

			// 否则说明定时器到期， 执行回调函数
			(*itr)->timer.handlerTimeOut();
			
			auto temp_itr = itr++;

			node = *temp_itr;

			// 删除定时器
			delete node;
			slot_list->erase(temp_itr);

		}
	}
	_cur_slot = (_cur_slot + 1 ) % _SLOTS_NUM;
}

template <typename _UData>
TimerNode<_UData> * TWTimerContainer<_UData>::del_timer(Timer<_UData> *timer){

	// 由于Timer在TimerNode中第一个位置，可以直接强转
	TimerNode<_UData> *timer_node = reinterpret_cast<TimerNode<_UData> *> (timer);
	if(timer_node == nullptr){
		return nullptr;
	}

	// 获取定时器在时间轮中的哪个槽中
	int ts = timer_node->getTimeSlot();
	auto slot_list = _slots[ts];
	if(slot_list ==  nullptr){
		return nullptr;
	}

	slot_list->remove(timer_node);

	return timer_node;

}

template <typename _UData>
void TWTimerContainer<_UData>::add_timer(TimerNode<_UData> *timer_node, time_t timeout)
{
    /*  
        根据待插入定时器的超时值计算出它经过多少个时间滴答后被触发。
        如果传入的时间值小于时间轮的槽间隔，则向上折合
    */

    int ticks = 0;
    // 计算出定时时间需要走过的槽数
    if(timeout < _SI)
    {
        ticks = 1;
    }   
    else
    {
        ticks = timeout / _SI;
    }
                 
    int rotation = ticks / _SLOTS_NUM;        // 圈数
    int ts = (_cur_slot + (ticks % _SLOTS_NUM)) % _SLOTS_NUM;      // 计算待插入的定时器应该被插入哪个槽中                            
    
    timer_node->setRotation(rotation);
    timer_node->setTimeSlot(ts);

    if(_slots[ts] == nullptr)
    {
        _slots[ts] = new TimerList;
    }

    _slots[ts]->push_back(timer_node);

    std::cout << "add timer, rotation:" << rotation << " ts:" << ts << std::endl;
}

// 添加一个定时器，并返回Timer类型指针
template <typename _UData>    
Timer<_UData> *TWTimerContainer<_UData>::addTimer(time_t timeout)
{                               
    TimerNode<_UData> *timer_node = new TimerNode<_UData>;
    
    if(timer_node)
    {
        add_timer(timer_node, timeout);
        return &timer_node->timer;
    }
    
    return nullptr;
}

// 删除一个定时器
template <typename _UData>
void TWTimerContainer<_UData>::delTimer(Timer<_UData> *timer)
{
    TimerNode<_UData> *timer_node = del_timer(timer);
    if(timer_node)
    {
        delete timer_node;
    }
    
}

template <typename _UData>
void TWTimerContainer<_UData>::resetTimer(Timer<_UData> *timer, time_t timeout) 
{
    TimerNode<_UData> *timer_node = del_timer(timer);
    if(!timer_node)
    {
        return ;
    }

    add_timer(timer_node, timeout);
}

template <typename _UData>
int TWTimerContainer<_UData>::getMinExpire()
{
    return _SI;
}


#endif
















