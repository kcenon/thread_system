/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#pragma once

#include <string>

namespace utility_module
{
	class convert_string
	{
	public:
		static auto to_string(const std::wstring& message) -> std::string;
		static auto to_string(const std::u16string& message) -> std::string;
		static auto to_string(const std::u32string& message) -> std::string;

		static auto to_wstring(const std::string& message) -> std::wstring;
		static auto to_wstring(const std::u16string& message) -> std::wstring;
		static auto to_wstring(const std::u32string& message) -> std::wstring;

		static auto to_u16string(const std::string& message) -> std::u16string;
		static auto to_u16string(const std::wstring& message) -> std::u16string;
		static auto to_u16string(const std::u32string& message) -> std::u16string;

		static auto to_u32string(const std::string& message) -> std::u32string;
		static auto to_u32string(const std::wstring& message) -> std::u32string;
		static auto to_u32string(const std::u16string& message) -> std::u32string;

	private:
		template <typename From, typename To>
		static auto convert(std::basic_string_view<typename From::value_type> from) -> To;
	};
}