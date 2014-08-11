/**
 * Copyright (c) 2004-2007, Pavel Kraynyukhov.
 *  
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted under the terms of the General Public License version 2
 * (GPLv2), provided that the above copyright notice and this paragraph and the
 * following two paragraphs and the "LICENSE" file appear in all modified or
 * unmodified copies of the software "AS IS" and without any changes.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 * 
 * 
 * $Id: Singleton.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <memory>
#include <Val2Type.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <sys/Nanosleep.h>
#include <boost/shared_ptr.hpp>

namespace itc {

    /**
     * \brief Thread safe singleton template
     * Define here, necessary template methods, with more then 2 arguments
     * if required. Note that use of this class can syncronize your threads
     * because of mutex locking.
     * 
     **/
    template<class T> class Singleton {
    private:

        explicit Singleton() // Elliminate any instantiations of this class.
        {
            throw std::bad_alloc();
        }

    protected:
        virtual ~Singleton() = 0;

    public:

        static std::shared_ptr<T> getInstance() {
            itc::sys::SyncLock sync(mMutex);

            if (mInstance.get()) {
                return mInstance;
            } else {
                std::shared_ptr<T> tmp(new T());
                mInstance.swap(tmp);
                return mInstance;
            }
        }

        template <typename T1> static std::shared_ptr<T> getInstance(T1 arg) {
            itc::sys::SyncLock sync(mMutex);

            if (mInstance.get()) {
                return mInstance;
            } else {
                std::shared_ptr<T> tmp(new T(arg));
                mInstance.swap(tmp);
                return mInstance;
            }
        }

        template <typename T1, typename T2> static std::shared_ptr<T> getInstance(T1 arg1, T2 arg2) {
            itc::sys::SyncLock sync(mMutex);

            if (mInstance.get()) {
                return mInstance;
            } else {
                std::shared_ptr<T> tmp(new T(arg1, arg2));
                mInstance.swap(tmp);
                return mInstance;
            }
        }

        static bool tryDestroyInstance() {
            itc::sys::SyncLock sync(mMutex);
            if (mInstance.unique()) {
                std::shared_ptr<T> tmp((T*) 0);
                mInstance.swap(tmp);
                return true;
            }
            return false;
        }

        static void destroyInstance() {
            itc::sys::SyncLock sync(mMutex);
            itc::sys::Sleep aTimer;
            while ((!mInstance.unique()) && mInstance.use_count()) {
                aTimer.usleep(10);
            }
            std::shared_ptr<T> tmp((T*) 0);
            mInstance.swap(tmp);
        }

    protected:
        static itc::sys::Mutex mMutex;
        static std::shared_ptr<T> mInstance;
    };

    // Initialisation
    template <typename T> itc::sys::Mutex Singleton<T>::mMutex;
    template <typename T> std::shared_ptr<T> Singleton<T>::mInstance((T*) 0);
}

#endif
