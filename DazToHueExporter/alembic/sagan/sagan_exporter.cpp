/*
COPYRIGHT (C) 2020-2025, I WAS SERIOUS PRODUCTIONS
ALL RIGHTS RESERVED.

REDISTRIBUTION AND USE IN SOURCE AND BINARY FORMS, WITH OR WITHOUT
MODIFICATION, ARE PERMITTED PROVIDED THAT THE FOLLOWING CONDITIONS ARE MET:
1. REDISTRIBUTIONS OF SOURCE CODE MUST RETAIN THE ABOVE COPYRIGHT
   NOTICE, THIS LIST OF CONDITIONS AND THE FOLLOWING DISCLAIMER.
2. REDISTRIBUTIONS IN BINARY FORM MUST REPRODUCE THE ABOVE COPYRIGHT
   NOTICE, THIS LIST OF CONDITIONS AND THE FOLLOWING DISCLAIMER IN THE
   DOCUMENTATION AND/OR OTHER MATERIALS PROVIDED WITH THE DISTRIBUTION.
3. ALL ADVERTISING MATERIALS MENTIONING FEATURES OR USE OF THIS SOFTWARE
   MUST DISPLAY THE FOLLOWING ACKNOWLEDGEMENT:
   THIS PRODUCT INCLUDES SOFTWARE DEVELOPED BY I WAS SERIOUS PRODUCTIONS.
4. NEITHER THE NAME OF I WAS SERIOUS PRODUCTIONS NOR THE
   NAMES OF ITS CONTRIBUTORS MAY BE USED TO ENDORSE OR PROMOTE PRODUCTS
   DERIVED FROM THIS SOFTWARE WITHOUT SPECIFIC PRIOR WRITTEN PERMISSION.

THIS SOFTWARE IS PROVIDED BY I WAS SERIOUS PRODUCTIONS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL I WAS SERIOUS PRODUCTIONS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <QApplication>
#include <QtCore/QTextStream>

#include "sagan_exporter.h"

#include "dzscene.h"
#include "dzfigure.h"
#include "dzbone.h"
#include "dzfacetmesh.h"
#include "dzfacegroup.h"

#include <stdexcept>
#include <array>

Sagan::SaganExporter::SaganExporter(const OutputTransformer* outputTransformer) : m_outputTransformer(outputTransformer)
{
}

const Sagan::OutputTransformer* Sagan::SaganExporter::getOutputTransformer() const
{

	return m_outputTransformer;

}

const Alembic::Abc::TimeSamplingPtr Sagan::SaganExporter::getTimeSampling() const
{

	return m_TimeSampling;

}

const Alembic::Abc::OArchive* Sagan::SaganExporter::getArchive() const
{

	return m_Archive.get();

}

Alembic::Abc::OArchive* Sagan::SaganExporter::getArchive()
{

	return m_Archive.get();

}

Sagan::AlembicMeshObjects& Sagan::SaganExporter::getAlembicMeshObjects()
{

	return m_AlembicMeshObjects;

}

const Sagan::AlembicMeshObjects& Sagan::SaganExporter::getAlembicMeshObjects() const
{

	return m_AlembicMeshObjects;

}

Sagan::ExportableMeshObjectPtrs& Sagan::SaganExporter::getExportableMeshObjects()
{

	return m_exportableMeshObjects;

}

const Sagan::ExportableMeshObjectPtrs& Sagan::SaganExporter::getExportableMeshObjects() const
{

	return m_exportableMeshObjects;

}
