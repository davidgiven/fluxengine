#ifndef GUI_H
#define GUI_H

class Showable
{
public:
	virtual ~Showable() {}

public:
	virtual void show() = 0;
	virtual void hide() = 0;
};

extern std::unique_ptr<Showable> createMainApp();

#endif

