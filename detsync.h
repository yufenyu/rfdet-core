/*
 * DetSync.h
 * Implementing deterministic synchronization order.
 *  Created on: May 8, 2013
 *      Author: zhouxu
 */

#ifndef DETSYNC_H_
#define DETSYNC_H_

class DetSync {
public:
	DetSync();
	virtual ~DetSync();
public:
	static int detLock(void* l);
	static int detTrylock(void* l);
	static int detUnlock(void* l);
};

#endif /* DETSYNC_H_ */
