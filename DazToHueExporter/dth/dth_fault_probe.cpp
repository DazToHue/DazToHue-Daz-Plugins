#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#include <string.h>

#include "dth/dth_fault_probe.h"

namespace
{

	// Fixed buffers, not QString: the breadcrumb is written on the hot path
	// (once per node per frame) and read immediately after a hard fault, when
	// allocating is the last thing worth doing.
	const int kStageMax = 64;
	const int kNodeMax = 160;

	char g_stage[kStageMax] = { 0 };
	char g_node[kNodeMax] = { 0 };
	int g_frame = -1;
	int g_nodeIndex = -1;

	void copyInto( char* destination, int destinationSize, const char* source )
	{
		if ( destination == nullptr || destinationSize <= 0 ) return;

		if ( source == nullptr )
		{
			destination[0] = 0;
			return;
		}

		strncpy_s( destination, destinationSize, source, _TRUNCATE );
	}

	// MSVC will not compile __try/__except into a function that also needs C++
	// object unwinding, so the filter lives here as a plain function and the
	// __try frame below keeps nothing but PODs.
	int faultFilter( unsigned long code, EXCEPTION_POINTERS* pointers, DthFaultProbe::FaultReport* report )
	{
		// A C++ exception is an SEH exception too (0xE06D7363, "msc"). Letting
		// this handler take it would break every existing catch in the export,
		// which is where a real runtime_error is supposed to be reported.
		if ( code == 0xE06D7363 ) return EXCEPTION_CONTINUE_SEARCH;

		if ( report == nullptr ) return EXCEPTION_CONTINUE_SEARCH;

		report->code = code;

		if ( pointers != nullptr && pointers->ExceptionRecord != nullptr )
		{
			const EXCEPTION_RECORD* record = pointers->ExceptionRecord;

			report->address = reinterpret_cast<unsigned long long>( record->ExceptionAddress );

			// For an access violation the record carries what was touched:
			// [0] is 0 read / 1 write / 8 DEP, [1] is the address.
			if ( code == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2 )
			{
				report->wasWrite = ( record->ExceptionInformation[0] == 1 );
				report->referenced = static_cast<unsigned long long>( record->ExceptionInformation[1] );
				report->haveReferenced = true;
			}
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	const char* codeName( unsigned long code )
	{
		switch ( code )
		{
		case EXCEPTION_ACCESS_VIOLATION:		return "ACCESS_VIOLATION";
		case EXCEPTION_STACK_OVERFLOW:			return "STACK_OVERFLOW";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:		return "INT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:		return "FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_ILLEGAL_INSTRUCTION:		return "ILLEGAL_INSTRUCTION";
		case EXCEPTION_PRIV_INSTRUCTION:		return "PRIV_INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR:			return "IN_PAGE_ERROR";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:	return "ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_DATATYPE_MISALIGNMENT:	return "DATATYPE_MISALIGNMENT";
		default:								return "hard fault";
		}
	}

}

bool DthFaultProbe::runProtected( const std::function<void()>& body, FaultReport& report )
{
	__try
	{
		body();
		return true;
	}
	__except ( faultFilter( GetExceptionCode(), GetExceptionInformation(), &report ) )
	{
		// Deliberately no cleanup here. Whatever the export was holding is
		// leaked or half-written, and that is the right trade: the process is
		// already in an unknown state, and the export set is discarded by the
		// caller (the .dth is never written, so the studio fails the run and
		// the previous set survives as .dthprev).
		return false;
	}
}

QString DthFaultProbe::describeFault( const FaultReport& report )
{
	QString text = QString( "%1 at 0x%2" )
		.arg( codeName( report.code ) )
		.arg( report.address, 16, 16, QChar( '0' ) );

	if ( report.haveReferenced )
	{
		text += QString( " %1 0x%2" )
			.arg( report.wasWrite ? "writing" : "reading" )
			.arg( report.referenced, 16, 16, QChar( '0' ) );
	}

	text += QString( " (code 0x%1)" ).arg( report.code, 8, 16, QChar( '0' ) );

	return text;
}

void DthFaultProbe::setStage( const char* stage )
{
	copyInto( g_stage, kStageMax, stage );
	g_frame = -1;
	g_nodeIndex = -1;
	g_node[0] = 0;
}

void DthFaultProbe::setFrame( int frame )
{
	g_frame = frame;
}

void DthFaultProbe::setNodeIndex( int index )
{
	g_nodeIndex = index;
}

void DthFaultProbe::setNode( const QString& label )
{
	copyInto( g_node, kNodeMax, label.toUtf8().constData() );
}

void DthFaultProbe::clear()
{
	g_stage[0] = 0;
	g_node[0] = 0;
	g_frame = -1;
	g_nodeIndex = -1;
}

QString DthFaultProbe::describeBreadcrumb()
{
	if ( g_stage[0] == 0 ) return QString( "no stage recorded" );

	QString text = QString::fromUtf8( g_stage );

	if ( g_frame >= 0 ) text += QString( ", frame %1" ).arg( g_frame );

	if ( g_nodeIndex >= 0 )
	{
		text += QString( ", node %1" ).arg( g_nodeIndex );
		if ( g_node[0] != 0 ) text += QString( " '%1'" ).arg( QString::fromUtf8( g_node ) );
	}

	return text;
}
