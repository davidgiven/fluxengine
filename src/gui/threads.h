#ifndef THREADS_H
#define THREADS_H

extern void UIInitThreading();
extern void UIRunOnUIThread(std::function<void(void)> callback);
extern void UIStartAppThread(std::function<void(void)> callback);

#endif
