#pragma once

/*
	Daz Studio 4 (Qt 4.8) vs Daz Studio 6 (Qt 6).

	This header is the ONLY place in the tree that knows which generation it is
	being compiled for. DAZ_SDK_MAJOR_VERSION comes from CMake (4 or 6).

	Three differences genuinely need it - each one verified against the two
	SDK headers, not assumed:

	  1. The Documents folder. Qt dropped QDesktopServices::storageLocation in
	     Qt5, and QStandardPaths does not exist in Qt 4.8.
	  2. DzScript lifetime. SDK6's DzScript has a PROTECTED destructor
	     (dzscript.h: `~DzScript() override` under `protected:`) and is disposed
	     via DzBase::unref(); SDK4's destructor is public and its DzBase has no
	     ref()/unref() at all.
	  3. DzNode::isVisib[i]leInRender. SDK4 spells it `isVisibileInRender`
	     (dznode.h:196) and has no other spelling; SDK6 has the corrected
	     `isVisibleInRender` (dznode.h:350) and keeps the typo only as a
	     deprecated alias marked "TODO : SDK Next : remove" (dznode.h:453).

	Everything else that LOOKS like it needs a shim does not, and the sources
	are written the portable way instead:

	  - DzSkeleton::getAllBones() returns QObjectList on SDK4 but DzBoneList on
	    SDK6 - however BOTH ship the getAllBones(DzBoneList&) out-parameter
	    overload, so the call sites use that.
	  - DzScript::result() is QVariant on SDK4 and QJSValue on SDK6; both have
	    .toString(), so `auto` covers the difference at every CALL SITE. Only
	    the declaration of that type is version-specific, and this header
	    includes the right one (dzscript.h merely forward-declares QJSValue).
	  4. Invoking a DTH json script. SDK6's dzscript.h DECLARES
	     DzScript::call(function, args), but dzcore.dll does not EXPORT it -
	     using it fails at link with LNK2019 (measured, not assumed). So SDK6
	     runs the script program and lets it return the value, while SDK4 keeps
	     the call("execute") it has always used. One visibility.dsa serves both.
	  - DzFileIOSettings has a protected destructor on SDK6 and so must be
	    heap-allocated; `new` is equally valid on SDK4, so both use the heap.
	  - Module-less Qt includes (<QMessageBox>) resolve under Qt4 and Qt6 alike,
	    given the module include directories the CMake Qt targets carry.
*/

#include <QtCore/QString>

#include "dznode.h"
#include "dzscript.h"

#if DAZ_SDK_MAJOR_VERSION >= 6
	#include <QStandardPaths>
	#include <QJSValue>			// what DzScript::result() returns on SDK6
#else
	#include <QtGui/QDesktopServices>
	#include <QtCore/QVariant>	// what DzScript::result() returns on SDK4
#endif

namespace DthCompat
{

	/** The current user's Documents folder. */
	inline QString documentsPath()
	{
#if DAZ_SDK_MAJOR_VERSION >= 6
		return QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation );
#else
		return QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation );
#endif
	}

	/** Claim ownership of a freshly constructed DzScript. Pair with disposeScript(). */
	inline void retainScript( DzScript* script )
	{
#if DAZ_SDK_MAJOR_VERSION >= 6
		if ( script ) script->ref();
#else
		Q_UNUSED( script );
#endif
	}

	/** Release a DzScript claimed with retainScript(). */
	inline void disposeScript( DzScript* script )
	{
		if ( !script ) return;
#if DAZ_SDK_MAJOR_VERSION >= 6
		script->unref();	// ref-counted; the destructor is protected
#else
		delete script;		// public virtual destructor, no ref counting
#endif
	}

	/**
		Run a DTH json script - one whose body is `function execute() { ...
		return json; }` - and hand back what it returned. False if the script
		could not be run.

		Both generations end up doing the same thing; only the route differs.
		SDK4 calls the function by name. SDK6 cannot (dzcore does not export
		DzScript::call), so the program gets a `return execute();` appended and
		is run as a whole - which is exactly what the DS6 tree did before the
		merge, just with the body in a function instead of at top level.
	*/
	inline bool runJsonScript( const QString& source, QString& json )
	{
		DzScript* script = new DzScript( "DTHScript" );
		retainScript( script );

		bool ok = false;

#if DAZ_SDK_MAJOR_VERSION >= 6
		script->setCode( source + "\nreturn execute();\n" );
		ok = script->execute( QVariantList() );
#else
		script->setCode( source );
		ok = script->call( "execute", QVariantList() );
#endif

		if ( ok )
		{
			// `auto`: QVariant on SDK4, QJSValue on SDK6. Both have toString().
			auto result = script->result();
			json = result.toString();
		}

		disposeScript( script );

		return ok;
	}

	/** Whether a node is visible in renders. */
	inline bool isVisibleInRender( const DzNode* node )
	{
		if ( !node ) return false;
#if DAZ_SDK_MAJOR_VERSION >= 6
		return node->isVisibleInRender();
#else
		return node->isVisibileInRender();	// SDK4 has only the typo'd spelling
#endif
	}

}
