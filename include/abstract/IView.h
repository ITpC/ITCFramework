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
 * $Id:$
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 **/


#ifndef  __IVIEW_H__
#define	__IVIEW_H__

namespace itc {
    namespace abstract {
        template <typename TModel> class IView
        {
        public:
            void update(const TModel& pM)
            {
                onUpdate(pM);
            }
        
        protected:
            virtual void onUpdate(const TModel&)=0;
            
            virtual ~IView()=default;
        };
    }
}


#endif	/* __IVIEW_H__ */
