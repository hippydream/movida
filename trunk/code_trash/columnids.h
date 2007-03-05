/****************************************************************************
** Filename: columnids.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Provides application-wide column identifiers for Movida.
**
** Copyright (C) 2006 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.sourceforge.net/).
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See the file LICENSE.GPL that came with this software distribution or
** visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
**********************************************************************/

#ifndef MOVIDA_COLUMNIDS__H
#define MOVIDA_COLUMNIDS__H

#include <QObject>
#include "dataview.h"

class MovidaCID : QObject
{
	Q_OBJECT

public:
	static const unsigned short Cast = 0;
	static const unsigned short Category = 1;
	static const unsigned short ColorMode = 2;
	static const unsigned short Comment = 3;
	static const unsigned short Countries = 4;
	static const unsigned short Country = 5;
	static const unsigned short Crew = 6;
	static const unsigned short Description = 7;
	static const unsigned short Directors = 8;
	static const unsigned short Firstname = 9;
	static const unsigned short Genre = 10;
	static const unsigned short Genres = 11;
	static const unsigned short Id = 13;
	static const unsigned short ImdbId = 14;
	static const unsigned short Lastname = 15;
	static const unsigned short Length = 16;
	static const unsigned short Links = 17;
	static const unsigned short Location = 18;
	static const unsigned short MediaCount = 19;
	static const unsigned short MediaType = 20;
	static const unsigned short StorageType = 21;
	static const unsigned short LogMessage = 22;
	static const unsigned short Name = 24;
	static const unsigned short Notes = 25;
	static const unsigned short OriginalTitle = 26;
	static const unsigned short Plot = 27;
	static const unsigned short Producers = 29;
	static const unsigned short ProductionYear = 30;
	static const unsigned short Rating = 31;
	static const unsigned short ReleaseYear = 32;
	static const unsigned short Resolution = 33;
	static const unsigned short Role = 34;
	static const unsigned short Title = 35;
	static const unsigned short Type = 37;
	static const unsigned short Url = 38;
	static const unsigned short Version = 39;

	static QString label(unsigned short cid)
	{
		switch (cid)
		{
			case Cast : return tr("Cast");
			case Category : return tr("Category");
			case ColorMode : return tr("Color mode");
			case Comment : return tr("Comment");
			case Countries : return tr("Countries");
			case Country : return tr("Country");
			case Crew : return tr("Crew");
			case Description : return tr("Description");
			case Directors : return tr("Directors");
			case Firstname : return tr("First name");
			case Genre : return tr("Genre");
			case Genres : return tr("Genres");
			case Id : return tr("Id");
			case ImdbId : return tr("IMDB Id");
			case Lastname : return tr("Last name");
			case Length : return tr("Length");
			case Links : return tr("Links");
			case Location : return tr("Location");
			case MediaCount : return tr("Media count");
			case MediaType : return tr("Media type");
			case StorageType : return tr("Storage type");
			case LogMessage : return tr("Log message");
			case Name : return tr("Name");
			case Notes : return tr("Notes");
			case OriginalTitle : return tr("Original title");
			case Plot : return tr("Plot");
			case Producers : return tr("Producers");
			case ProductionYear : return tr("Production year");
			case Rating : return tr("Rating");
			case ReleaseYear : return tr("Release year");
			case Resolution : return tr("Resolution");
			case Role : return tr("Role");
			case Title : return tr("Title");
			case Type : return tr("Type");
			case Url : return tr("URL");
			case Version : return tr("Edition");
			default: ;
		}

		return QString();
	}

	static DataView::ColumnType type(unsigned short cid)
	{
		switch (cid)
		{
			case Cast :
			case Category :
			case ColorMode :
			case Comment :
			case Countries :
			case Country :
			case Crew :
			case Description :
			case Directors :
			case Firstname :
			case Genre :
			case Genres :
			case Lastname :
			case Links :
			case Location :
			case MediaType :
			case StorageType :
			case Name :
			case Notes :
			case OriginalTitle :
			case Plot :
			case Producers :
			case Role :
			case Title :
			case Type :
			case Url :
			case Version :
				return DataView::TextData;

			case ProductionYear :
			case Rating :
			case ReleaseYear :
			case Id :
			case ImdbId :
			case MediaCount :
				return DataView::NumberData;

			case Length :
				return DataView::TimeData;
			
			case Resolution :
				return DataView::ResolutionData;

			case LogMessage :
				return DataView::IDColumn;

			default: ;
		}

		return DataView::TextData;
	}
};

#endif // MOVIDA_COLUMNIDS__H
