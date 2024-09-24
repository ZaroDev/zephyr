#pragma once

namespace Zephyr::FileDialogs
{
	Zephyr::Path OpenFile(Zephyr::StrView filter);
	Zephyr::Path SaveFile(Zephyr::StrView filter);
}