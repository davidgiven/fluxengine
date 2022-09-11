#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

class LocalBase;

class Environment
{
public:
    static void reset();
    static void addVariable(LocalBase* local);
};

class LocalBase
{
public:
    virtual void reset() = 0;
};

template <class T>
class Local : LocalBase
{
public:
    Local()
    {
        Environment::addVariable(this);
    }
    void reset() override
    {
        _value.clear();
    }
    T& operator*()
    {
        return _value;
    }
    T& operator->()
    {
        return _value;
    }

private:
    T _value;
};

#endif
