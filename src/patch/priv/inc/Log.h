/*
 * Copyright (c) 2020 EKA2L1 Team.
 * 
 * This file is part of EKA2L1 project.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MEDIA_CLIENT_AUDIO_STREAM_LOG_H_
#define MEDIA_CLIENT_AUDIO_STREAM_LOG_H_

#include <e32std.h>

_LIT(KMcaCat, "MediaClientAudioStream");
_LIT(KMcvCat, "MediaClientVideo");
_LIT(KBacklightCat, "Backlight");
_LIT(KCameraCat, "Camera");
_LIT(KScdvCat, "SCDV-HLE");
_LIT(KPostingSurfaceCat, "PostingSurface");

void LogOut(const TDesC &aCategory, const TDesC &aMessage, ...);

#endif
