#pragma once

#include <functional>

#include <QString>

/*
	Locating a failure that is NOT a C++ exception.

	The Alembic leg is contained twice already - the frame loop catches and
	names the frame (alembic_exporter.cpp), and every Q_INVOKABLE routes
	through runGuardedExport() (dth_exporter_action.cpp). Both are C++ catches,
	and on 2026-08-24 a death slipped past both: two runs of one 483-frame ROM
	(Ita_G9_GP, DS4, this 2.1.4 build) stopped mid frame loop having logged
	NOTHING from either handler - no "failed at frame N", no
	"DazToHue Exporter: doExport failed". Daz logged its own

	    dzscript.cpp(1192): Unhandled error while executing script.
	    QScriptEngine::popContext() doesn't match with pushContext()

	and carried on. A C++ catch cannot contain a hard fault: under /EHsc an
	access violation is not a C++ exception, so nothing we write as `catch`
	will ever see it. Daz's own SEH wrapper around script execution is what
	swallowed it, which is also why the process survived and Windows recorded
	neither an event nor a crash dump.

	Two pieces, and they are only useful together:

	  - runProtected() puts a __try/__except around the export so the fault is
	    caught HERE, with its code and address, instead of vanishing into Daz.
	    The filter deliberately passes C++ exceptions (0xE06D7363) through, so
	    the existing catches keep working exactly as before.
	  - the breadcrumb records what the export was doing - stage, frame, node -
	    updated as it goes and read only after a fault. A fault address alone
	    names a function; the breadcrumb names the DATA, which is what a
	    dangling DzNode* looks like from the outside.

	Both are cheap enough for the shipped build (the breadcrumb is a strncpy
	per node per frame) and that is on purpose: this failure is intermittent
	and was not reproduced on demand, so it has to be caught in the build that
	is actually running when it happens.
*/

namespace DthFaultProbe
{

	struct FaultReport
	{
		unsigned long code = 0;			// EXCEPTION_ACCESS_VIOLATION, ...
		unsigned long long address = 0;		// faulting instruction
		unsigned long long referenced = 0;	// address the access violation touched
		bool wasWrite = false;
		bool haveReferenced = false;
	};

	// Runs body() under __try/__except. Returns true when it completed, false
	// when a hard fault was caught (report is filled in then). C++ exceptions
	// are NOT caught here - they propagate to the caller's catch as before.
	bool runProtected( const std::function<void()>& body, FaultReport& report );

	// Human-readable "ACCESS_VIOLATION reading 0x0000000000000018 at 0x00007ff8..."
	QString describeFault( const FaultReport& report );

	// The breadcrumb. Set as the export proceeds, read after a fault.
	void setStage( const char* stage );
	void setFrame( int frame );
	void setNodeIndex( int index );
	void setNode( const QString& label );
	void clear();

	// "alembic ROM, frame 137, node 12 'CHT Sevenly Hair'"
	QString describeBreadcrumb();

}
