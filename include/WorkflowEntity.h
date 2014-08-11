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
 * $Id: WorkflowEntity.h 22 2010-11-23 12:53:33Z pk $
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 **/

#ifndef IWORKFLOWENTITY_H_
#define IWORKFLOWENTITY_H_
#include <compat_types.h>

namespace itc {
    namespace workflow {

        /**
         * Base class for any workflow entities.
         *
         **/
        class WorkflowEntity {
        private:
            uint32_t mId;
            uint32_t mPrevStateId;
            uint32_t mNextStateId;
            uint32_t mErrorStateId;
        public:

            explicit WorkflowEntity(
                    const uint32_t id, const uint32_t pPrevState,
                    const uint32_t pNextState, const uint32_t mErrorState
            )
            :   mId(id), mPrevStateId(pPrevState),
                mNextStateId(pNextState),
                mErrorStateId(mErrorState)
            {
            }

            explicit WorkflowEntity(const WorkflowEntity& ref)
            :   mId(ref.mId), mPrevStateId(ref.mPrevStateId),
                mNextStateId(ref.mNextStateId),
                mErrorStateId(ref.mErrorStateId)
            {
            }

            ~WorkflowEntity() {
            }

            inline void setPrevStateId(const uint32_t id) {
                mPrevStateId = id;
            }

            inline void setNextStateId(const uint32_t id) {
                mNextStateId = id;
            }

            inline void setErrorStateId(const uint32_t id) {
                mErrorStateId = id;
            }

            inline const uint32_t getNextStateId() const {
                return mNextStateId;
            }

            inline const uint32_t getPrevStateId() const {
                return mNextStateId;
            }

            inline const uint32_t getErrorStateId() const {
                return mNextStateId;
            }

            inline const uint32_t getId() const {
                return mId;
            }
        };
    }
}

#endif /*IWORKFLOWENTITY_H_*/
