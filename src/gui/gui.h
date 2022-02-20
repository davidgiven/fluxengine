#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

extern void runOnUiThread(std::function<void()> callback);
extern void runOnWorkerThread(std::function<void()> callback);

template <typename R>
static inline R runOnUiThread(std::function<R()>& callback)
{
	R retvar;
	runOnUiThread(
		[&]() {
			retvar = callback();
		}
	);
	return retvar;
}

#endif

