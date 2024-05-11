#pragma once
#include<iostream>
#include<memory>
#include<mutex>
template<typename T>
class Sigleton
{
public:
    static T& GetInstance()
    {
        static T _instance;
        return _instance;
    }

protected:
    Sigleton() {};
    ~Sigleton() {};
private:
    Sigleton(const Sigleton&) = delete;
    Sigleton& operator=(const Sigleton&) = delete;
};