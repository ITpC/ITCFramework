/**
 * Copyright (c) 2007, Pavel Kraynyukhov.
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
 * $Id: RScheduler.h 22 2015-02-16 19:00:33Z pk $
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 **/

#ifndef RSCHEDULER_H
#define	RSCHEDULER_H


#include <stdint.h>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <list>
#include <abstract/Runnable.h>
#include <sys/Mutex.h>
#include <sys/SyncLock.h>
#include <InterfaceCheck.h>
#include <CollectibleThread.h>
#include <cmath>

namespace itc
{
    template <typename T> class RScheduler : public itc::abstract::IRunnable
    {
    private:
        typedef std::shared_ptr<T> storable;
        typedef std::list<storable> storable_container;
        typedef std::map<uint32_t, storable_container> map_type;
        typedef CollectibleThread<T> ThreadType;
        typedef std::shared_ptr<ThreadType> storable_thread;
        typedef std::list<storable_thread>    tclist;

        itc::sys::Mutex mMutex;
        map_type        mSchedule;
        bool            mDoRun;
        size_t          mNextWake;
        tclist          mTrashcan;

    public:
        RScheduler()
        {
            STATIC_CHECKER3MSG(
               CheckRelationship(
                 T, subclassof, gcClient
                 ),
               T, _is_not_a_subclass_of_, gcClient
            );
        }

        const RScheduler& add(uint32_t delay, const storable& ref)
        {
            if(delay == 0)
            {
                throw std::bad_alloc(); // TODO: replace with AppException
            }

            itc::sys::SyncLock  synchronize(mMutex);
            std::map::iterator it=mSchedule.find(delay);
            if(it!=mSchedule.end())
            {
                it->second.insert(ref);
            }
            else
            {
                storable_container tmp;
                tmp.push_back(ref);
                mSchedule.insert(std::pair<uint32_t,storable_container>(delay,tmp));
            }
            size_t first_wake=mSchedule.begin()->first*1000;

            if(mNextWake>first_wake)
            {
                mNextWake=first_wake;
            }
        }

        bool mayRun()
        {
            itc::sys::SyncLock  synchronize(mMutex);
            return mDoRun;
        }

        void execute()
        {
            while(mayRun())
            {
                try {
                    itc::sys::SyncLock  synchronize(mMutex);
                    if(!mSchedule.empty())
                    {
                        if(mNextWake > 100)
                            mNextWake-=100;
                        else
                        {
                            storable_container::iterator runnableIt=mSchedule.begin()->second.begin();
                            
                            while(runnableIt!=mSchedule.begin()->second.end())
                            {
                                
                            }
                            
                            mTrashcan.push_back(std::make_shared<ThreadType>(*(mSchedule.begin())));
                            map_type::iterator it=mSchedule.begin();
                            uint32_t prevdelay=it->first*1000;
                            mSchedule.erase(it);
                            mNextWake=abs(mSchedule.begin()->first*1000 - prevdelay);
                        }
                    }
                }catch(std::exception& e)
                {

                }
                
                for(tclist::iterator it=mTrashcan.begin();it!=mTrashcan.end();it++)
                {
                    if(it->canRemove())
                    {
                        mTrashcan.erase(it);
                    }
                }
                usleep(100);
            }
        }
    };
}


#endif	/* RSCHEDULER_H */

