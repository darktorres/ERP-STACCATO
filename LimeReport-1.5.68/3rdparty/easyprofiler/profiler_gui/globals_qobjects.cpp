/************************************************************************
 * file name         : globals_qobjects.cpp
 * ----------------- :
 * creation time     : 2016/08/08
 * authors           : Victor Zarubkin, Sergey Yagovtsev
 * email             : v.s.zarubkin@gmail.com, yse.sey@gmail.com
 * ----------------- :
 * description       : The file contains implementation of EasyGlobalSignals QObject class.
 * ----------------- :
 * change log        : * 2016/08/08 Sergey Yagovtsev: moved sources from globals.cpp
 *                   :
 *                   : *
 * ----------------- :
 * license           : Lightweight profiler library for c++
 *                   : Copyright(C) 2016-2017  Sergey Yagovtsev, Victor Zarubkin
 *                   :
 *                   : Licensed under either of
 *                   :     * MIT license (LICENSE.MIT or http://opensource.org/licenses/MIT)
 *                   :     * Apache License, Version 2.0, (LICENSE.APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 *                   : at your option.
 *                   :
 *                   : The MIT License
 *                   :
 *                   : Permission is hereby granted, free of charge, to any person obtaining a copy
 *                   : of this software and associated documentation files (the "Software"), to deal
 *                   : in the Software without restriction, including without limitation the rights
 *                   : to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *                   : of the Software, and to permit persons to whom the Software is furnished
 *                   : to do so, subject to the following conditions:
 *                   :
 *                   : The above copyright notice and this permission notice shall be included in all
 *                   : copies or substantial portions of the Software.
 *                   :
 *                   : THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 *                   : INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *                   : PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *                   : LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *                   : TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 *                   : USE OR OTHER DEALINGS IN THE SOFTWARE.
 *                   :
 *                   : The Apache License, Version 2.0 (the "License")
 *                   :
 *                   : You may not use this file except in compliance with the License.
 *                   : You may obtain a copy of the License at
 *                   :
 *                   : http://www.apache.org/licenses/LICENSE-2.0
 *                   :
 *                   : Unless required by applicable law or agreed to in writing, software
 *                   : distributed under the License is distributed on an "AS IS" BASIS,
 *                   : WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *                   : See the License for the specific language governing permissions and
 *                   : limitations under the License.
 ************************************************************************/

#include "globals_qobjects.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

namespace profiler_gui {

EasyGlobalSignals::EasyGlobalSignals() : QObject() {}

EasyGlobalSignals::~EasyGlobalSignals() {}

} // END of namespace profiler_gui.

//////////////////////////////////////////////////////////////////////////
