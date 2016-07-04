#include "ThreadMutex.h"

CThreadMutex::CThreadMutex() {
	m_bIsValid = false;
    if (0 == pthread_mutex_init(&m_cs, NULL)) {
    	m_bIsValid = true;
    }
}

CThreadMutex::~CThreadMutex() {
	if (m_bIsValid) {
		pthread_mutex_destroy(&m_cs);
	}    
}

bool
CThreadMutex::lock() {
	if (m_bIsValid) {
    	return 0 == pthread_mutex_lock(&m_cs);
	}
	return false;
}

void
CThreadMutex::unlock() {
	if (m_bIsValid) {
    	pthread_mutex_unlock(&m_cs);
	}
}

bool
CThreadMutex::try_lock()
{
	if (m_bIsValid) {
		return 0 == pthread_mutex_trylock(&m_cs);
	}
	return false;
}


