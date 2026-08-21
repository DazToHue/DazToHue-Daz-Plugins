#pragma once

/*
	Daz Studio 4 (Qt 4.8) vs Daz Studio 6 (Qt 6).

	This header is the ONLY place in the tree that knows which generation it is
	being compiled for. DAZ_SDK_MAJOR_VERSION comes from CMake (4 or 6).

	Four differences genuinely need it - each one verified against the two
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
	  4. Raising an error the CALLING SCRIPT can catch. The Daz SDK offers no
	     script-error API of its own (nothing in dzapp.h / dzscript.h /
	     dpcscript.h), so this goes through the underlying engine, and the two
	     generations do not share one: DS4 is QtScript
	     (QScriptContext::throwError, qscriptcontext.h:85 - and the measured
	     crash message names QScriptEngine, so DS4's DzScript is confirmed to
	     host one), DS6 is QJSEngine (QJSEngine::throwError, qjsengine.h:323,
	     reached via qjsEngine(QObject*), qjsengine.h:424). See the
	     QScriptable note and raiseScriptError() below.

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
#include <QtCore/QObject>

#include "dznode.h"
#include "dzscript.h"

#if DAZ_SDK_MAJOR_VERSION >= 6
	#include <QStandardPaths>
	#include <QJSValue>			// what DzScript::result() returns on SDK6
	#include <QJSEngine>		// how SDK6 raises an error into the caller
#else
	#include <QtGui/QDesktopServices>
	#include <QtCore/QVariant>	// what DzScript::result() returns on SDK4
	#include <QtScript/QScriptable>
	#include <QtScript/QScriptContext>
#endif

/*
	QScriptable on SDK6.

	A class whose Q_INVOKABLE methods must be able to raise a catchable error
	names `public QScriptable` in its base clause (see
	dth_exporter_action.h). On SDK4 that is Qt's own class, included above,
	and the spelling is load-bearing: QtScript finds the subobject with
	qt_metacast("QScriptable"), and moc only emits that metacast entry for a
	base it sees under that literal name - so a typedef or a wrapper class
	would compile and then never be found at runtime.

	Qt6 has no QtScript, and SDK6 does not need it (QJSEngine is reached from
	the object itself) - but the base clause still has to PARSE on SDK6, and
	it cannot get there through a macro: moc does not expand one in a base
	clause. It silently drops the whole class instead ("No relevant classes
	found. No output generated."), the metaobject is never generated, and the
	build fails at the moc_*.cpp include - measured 2026-08-21, which is why
	the base is named directly rather than hidden behind one.

	So SDK6 gets an empty stand-in under the same name. Nothing queries it
	there. It is safe because Qt6 removed QtScript outright: there is no real
	QScriptable for it to collide with.
*/
#if DAZ_SDK_MAJOR_VERSION >= 6
	class QScriptable
	{
	};
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

	/**
		Raise `message` as an error in the script that is currently calling
		into us, so its own try/catch sees it. Call this from INSIDE a
		Q_INVOKABLE, on the object the script invoked, having already caught
		the C++ exception - a C++ exception must never unwind into the engine
		(that is the crash this exists to prevent).

		False means the error could not be handed to a script - either nothing
		scripted is calling (the dialog path, where the caller shows its own
		message box) or the engine did not expose us. Callers must treat false
		as "the caller will NOT learn about this from an exception" and say so
		in the log rather than assume it landed.
	*/
	inline bool raiseScriptError( QObject* invokedObject, const QString& message )
	{
		if ( !invokedObject ) return false;

#if DAZ_SDK_MAJOR_VERSION >= 6
		QJSEngine* engine = qjsEngine( invokedObject );
		if ( !engine ) return false;

		engine->throwError( message );
		return true;
#else
		// Exactly how QtScript itself finds a QScriptable subobject: a
		// metacast by name (see DTH_SCRIPTABLE_BASES above).
		void* scriptablePtr = invokedObject->qt_metacast( "QScriptable" );
		if ( !scriptablePtr ) return false;

		QScriptContext* context = reinterpret_cast<QScriptable*>( scriptablePtr )->context();
		if ( !context ) return false;	// not reached through a script

		context->throwError( message );
		return true;
#endif
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
