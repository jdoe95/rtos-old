/** *************************************************************************
 * @file
 * @brief Exported types
 * @author John Doe (jdoe35087@gmail.com)
 * @details
 * This file defines the types visible to the user.
 **************************************************************************/
#ifndef H16488323_48F4_461D_8B3F_D30921D74E5A
#define H16488323_48F4_461D_8B3F_D30921D74E5A

#include "config.h"

/**
 * @defgroup os_api_types API Types
 * @ingroup os_api
 * @brief API Types
 */

/**
 * @brief Thread state type
 * @ingroup os_api_types
 * @details This type defines the state of threads.
 */
typedef enum {
	OSTHREAD_READY = 0,		/**< @brief Ready or running */
	OSTHREAD_BLOCKED,		/**< @brief Blocked for timeout or for resources */
	OSTHREAD_SUSPENDED		/**< @brief Suspended */
} osThreadState_t;

/**
 * @brief Timer mode type
 * @ingroup os_api_types
 * @details This type defines the modes for @ref os_timer
 */
typedef enum {
	OSTIMERMODE_ONESHOT = 0,	/**< @brief One-shot mode */
	OSTIMERMODE_PERIODIC		/**< @brief Periodic mode */
} osTimerMode_t;

#endif /* H16488323_48F4_461D_8B3F_D30921D74E5A */
