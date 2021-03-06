/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

void GlobalScriptCompileBroadcaster::sendScriptCompileMessage(JavascriptProcessor *processorThatWasCompiled)
{
	if (!enableGlobalRecompile) return;

	for (int i = 0; i < listenerListStart.size(); i++)
	{
		if (listenerListStart[i].get() != nullptr)
		{
			listenerListStart[i].get()->scriptWasCompiled(processorThatWasCompiled);
		}
		else
		{
			listenerListStart.remove(i--);
		}
	}

	for (int i = 0; i < listenerListEnd.size(); i++)
	{
		if (listenerListEnd[i].get() != nullptr)
		{
			listenerListEnd[i].get()->scriptWasCompiled(processorThatWasCompiled);
		}
		else
		{
			listenerListEnd.remove(i--);
		}
	}
}

void GlobalScriptCompileBroadcaster::setCallStackEnabled(bool shouldBeEnabled)
{
	enableCallStack = shouldBeEnabled;

	ModulatorSynthChain *mainChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	Processor::Iterator<JavascriptProcessor> iter(mainChain);

	while (auto jp = iter.getNextProcessor())
	{
		jp->setCallStackEnabled(shouldBeEnabled);
	}
}

void GlobalScriptCompileBroadcaster::fillExternalFileList(Array<File> &files, StringArray &processors)
{
	ModulatorSynthChain *mainChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

	Processor::Iterator<JavascriptProcessor> iter(mainChain);

	while (JavascriptProcessor *sp = iter.getNextProcessor())
	{
		for (int i = 0; i < sp->getNumWatchedFiles(); i++)
		{
			if (!files.contains(sp->getWatchedFile(i)))
			{
				files.add(sp->getWatchedFile(i));
				processors.add(dynamic_cast<Processor*>(sp)->getId());
			}



		}
	}
}

void GlobalScriptCompileBroadcaster::setExternalScriptData(const ValueTree &collectedExternalScripts)
{
	externalScripts = collectedExternalScripts;
}

String GlobalScriptCompileBroadcaster::getExternalScriptFromCollection(const String &fileName)
{
    static const String deviceWildcard = "{DEVICE}";

    String realFileName = fileName;
    
    if(realFileName.contains(deviceWildcard))
    {
        realFileName = realFileName.replace(deviceWildcard, HiseDeviceSimulator::getDeviceName());
    }
    
	for (int i = 0; i < externalScripts.getNumChildren(); i++)
	{
        const String thisName = externalScripts.getChild(i).getProperty("FileName").toString();

        
		if (thisName == realFileName)
		{
			return externalScripts.getChild(i).getProperty("Content").toString();
		}
	}

	// Hitting this assert means you try to get a script that wasn't exported.
	jassertfalse;
	return String();
}

ExternalScriptFile::Ptr GlobalScriptCompileBroadcaster::getExternalScriptFile(const File& fileToInclude)
{
	for (int i = 0; i < includedFiles.size(); i++)
	{
		if (includedFiles[i]->getFile() == fileToInclude)
			return includedFiles[i];
	}

	includedFiles.add(new ExternalScriptFile(fileToInclude));

	return includedFiles.getLast();
}
