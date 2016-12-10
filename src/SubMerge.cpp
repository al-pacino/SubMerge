#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

using namespace std;

///////////////////////////////////////////////////////////////////////////////

typedef unsigned long int TTime;

class CTimeRange {
public:
	CTimeRange( TTime beginTime, TTime endTime ) :
		begin( beginTime ),
		end( endTime )
	{
		if( !( beginTime < endTime ) ) {
			throw invalid_argument(
				"The 'beginTime' should be less than the 'endTime'." );
		}
	}

	TTime Begin() const { return begin; }
	TTime End() const { return end; }

	bool HasIntersection( const CTimeRange& anotherTimeRange ) const;
	bool IsBefore( const CTimeRange& anotherTimeRange ) const;
	bool IsAfter( const CTimeRange& anotherTimeRange ) const;
	CTimeRange Union( const CTimeRange& anotherTimeRange ) const;

private:
	TTime begin;
	TTime end;
};

bool CTimeRange::HasIntersection( const CTimeRange& range ) const
{
	return !IsBefore( range ) && !IsAfter( range );
}

bool CTimeRange::IsBefore( const CTimeRange& anotherTimeRange ) const
{
	return ( end <= anotherTimeRange.begin );
}

bool CTimeRange::IsAfter( const CTimeRange& anotherTimeRange ) const
{
	return ( anotherTimeRange.end <= begin );
}

CTimeRange CTimeRange::Union( const CTimeRange& range ) const
{
	if( !HasIntersection( range ) ) {
		throw invalid_argument( "The uniting ranges should have intersection." );
	}

	return CTimeRange( min( begin, range.begin ), max( end, range.end ) );
}

///////////////////////////////////////////////////////////////////////////////

typedef unsigned long int TId;

class CSubtitle {
public:
	CSubtitle( TId subId ) :
		id( subId ),
		time( 0, 1 )
	{
	}

	TId Id() const { return id; }
	CTimeRange Time() const { return time; }
	const string& Text() const { return text; }

	void SetText( const string& subText )
	{
		text = subText;
	}

	void SetTime( CTimeRange subTime )
	{
		time = subTime;
	}

private:
	TId id;
	CTimeRange time;
	string text;
};

///////////////////////////////////////////////////////////////////////////////

class CSubtitles {
public:
	CSubtitles() {}

	bool Read( istream& input );
	bool ReadFromFile( const string& srtFilename );

	bool Write( ostream& output ) const;
	bool WriteToFile( const string& srtFilename ) const;

	const vector<CSubtitle>& Subtitles() const { return subtitles; }

	static bool CSubtitles::Merge( const CSubtitles& subtitles1,
		const CSubtitles& subtitles2, CSubtitles& subtitlesMerged );

private:
	vector<CSubtitle> subtitles;

	CSubtitles( const CSubtitles& );
	CSubtitles( const CSubtitles&& );
	CSubtitles& operator=( const CSubtitles& );
};

///////////////////////////////////////////////////////////////////////////////

bool CompareAndExtract( const string& format, const string& text, string& result )
{
	result.clear();
	if( format.length() != text.length() ) {
		return false;
	}

	for( string::size_type i = 0; i < format.length(); i++ ) {
		if( format[i] == '*' ) {
			result += text[i];
		} else if( format[i] != text[i] ) {
			return false;
		}
	}

	return true;
}

bool Fill( const string& format, const string& text, string& result )
{
	result = format;
	string::size_type j = 0;
	for( string::size_type i = 0; i < format.length(); i++ ) {
		if( result[i] == '*' ) {
			if( j == text.length() ) {
				return false;
			}
			result[i] = text[j];
			j++;
		}
	}

	return ( j == text.length() );
}

void ReadLine( istream& input, string& line )
{
	getline( input, line );
	if( !line.empty() && line.back() == '\r' ) {
		line.pop_back();
	}
}

bool ReadSubtitle( istream& input, CSubtitle& subtitle )
{
	string line;
	while( input.good() && line.empty() ) {
		ReadLine( input, line );
	}

	if( line.empty() ) {
		return false;
	}

	size_t pos;
	TId id;
	try {
		id = stoul( line, &pos );
	} catch( exception& ) {
		return false;
	}

	if( pos != line.length() || id == 0 ) {
		return false;
	}

	if( subtitle.Id() != id ) {
		return false;
	}

	ReadLine( input, line );
	string subTimes;
	if( !CompareAndExtract( "**:**:**,*** --> **:**:**,***", line, subTimes ) ) {
		return false;
	}

	TTime timeOfAppearing = stoul( subTimes.substr( 0, 9 ) );
	TTime timeOfDisappearing = stoul( subTimes.substr( 9, 9 ) );

	if( !( timeOfAppearing < timeOfDisappearing ) ) {
		return false;
	}

	string text;
	while( input.good() ) {
		ReadLine( input, line );
		if( line.empty() ) {
			break;
		}
		text += line + "\n";
	}

	if( text.empty() ) {
		return false;
	}

	subtitle.SetTime( CTimeRange( timeOfAppearing, timeOfDisappearing ) );
	subtitle.SetText( text );

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool CSubtitles::Read( istream& input )
{
	subtitles.clear();
	while( input.good() ) {
		subtitles.push_back( CSubtitle( subtitles.size() + 1 ) );
		if( !ReadSubtitle( input, subtitles.back() ) ) {
			break;
		}
	}

	if( subtitles.empty() ) {
		return false;
	}

	subtitles.pop_back();

	if( !input.eof() ) {
		return false;
	}

	return true;
}

bool CSubtitles::ReadFromFile( const string& srtFilename )
{
	ifstream file( srtFilename );
	return Read( file );
}

bool CSubtitles::Write( ostream& output ) const
{
	for( auto i = subtitles.cbegin(); i != subtitles.cend(); ++i ) {
		string textTime;
		ostringstream timeStream;
		timeStream << setw( 9 ) << setfill( '0' ) << i->Time().Begin();
		timeStream << setw( 9 ) << setfill( '0' ) << i->Time().End();
		if( !Fill( "**:**:**,*** --> **:**:**,***", timeStream.str(), textTime ) ) {
			return false;
		}
		output << i->Id() << endl;
		output << textTime << endl;
		output << i->Text() << endl;
	}

	return true;
}

bool CSubtitles::WriteToFile( const string& srtFilename ) const
{
	ofstream file( srtFilename, ios::out | ios::trunc );
	return Write( file );
}

bool CSubtitles::Merge( const CSubtitles& subtitles1,
	const CSubtitles& subtitles2, CSubtitles& subtitlesMerged )
{
	auto i1 = subtitles1.subtitles.cbegin();
	auto i2 = subtitles2.subtitles.cbegin();
	auto end1 = subtitles1.subtitles.cend();
	auto end2 = subtitles2.subtitles.cend();

	vector<CSubtitle>& subtitles = subtitlesMerged.subtitles;

	while( i1 != end1 && i2 != end2 ) {
		subtitles.push_back( CSubtitle( subtitles.size() + 1 ) );
		if( i1->Time().HasIntersection( i2->Time() ) ) {
			subtitles.back().SetTime( i1->Time().Union( i2->Time() ) );
			subtitles.back().SetText( i1->Text() + i2->Text() );
			++i1;
			++i2;
		} else if( i1->Time().IsBefore( i2->Time() ) ) {
			subtitles.back().SetTime( i1->Time() );
			subtitles.back().SetText( i1->Text() );
			++i1;
		} else {
			subtitles.back().SetTime( i2->Time() );
			subtitles.back().SetText( i2->Text() );
			++i2;
		}
	}

	if( i2 != end2 ) {
		i1 = i2;
		end1 = end2;
	}

	for( ; i1 != end1; ++i1 ) {
		subtitles.push_back( CSubtitle( subtitles.size() + 1 ) );
		subtitles.back().SetTime( i1->Time() );
		subtitles.back().SetText( i1->Text() );
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

int main( int argc, const char* const argv[] )
{
	if( argc != 4 ) {
		cout << "Usage:" << endl;
		cout << "  SubMerge SRT_FILE_1 SRT_FILE_1 SRT_FILE_MERGED" << endl;
		cout << endl;
		cout << "Developed by Anton Todua (c) 2016." << endl;
		return 1;
	}

	CSubtitles subtitiles1;
	if( !subtitiles1.ReadFromFile( argv[1] ) ) {
		cerr << "Error: Can't read the file `" << argv[1] << "`." << endl;
		return 1;
	}

	CSubtitles subtitiles2;
	if( !subtitiles2.ReadFromFile( argv[2] ) ) {
		cerr << "Error: Can't read the file `" << argv[2] << "`." << endl;
		return 1;
	}

	CSubtitles subtitilesMerged;
	if( !CSubtitles::Merge( subtitiles1, subtitiles2, subtitilesMerged ) ) {
		cerr << "Error: Can't merge the subtitles." << endl;
		return 1;
	}

	if( !subtitilesMerged.WriteToFile( argv[3] ) ) {
		cerr << "Error: Can't write the file `" << argv[3] << "`." << endl;
		return 1;
	}

	cout << "Done!" << endl;
	return 0;
}
