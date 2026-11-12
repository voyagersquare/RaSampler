#include "PluginProcessor.h"
#include "PluginEditor.h"
RaSamplerProcessor::RaSamplerProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),Thread("RaSampler stretch"),state(*this,nullptr,"RaSampler",ra::makeParameters()),engine(state){
 for(auto& m:mappings)m=-1;pending=std::make_unique<std::atomic<float>[]>(ra::specs().size());for(size_t i=0;i<ra::specs().size();++i)pending[i]=-1;original=playback=ra::makeInitSample();startThread();
}
RaSamplerProcessor::~RaSamplerProcessor(){signalThreadShouldExit();notify();stopThread(-1);cancelPendingUpdate();}
void RaSamplerProcessor::prepareToPlay(double rate,int size){engine.prepare(rate,size);}
void RaSamplerProcessor::releaseResources(){engine.reset();}
bool RaSamplerProcessor::isBusesLayoutSupported(const BusesLayout& l)const{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainInputChannelSet().isDisabled();}
float RaSamplerProcessor::value(const juce::String& id)const{if(auto* p=state.getRawParameterValue(id))return p->load();return 0;}
void RaSamplerProcessor::setParameter(const juce::String& id,float v){if(auto* p=state.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();}}
void RaSamplerProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& midi){
 keyboard.processNextMidiBuffer(midi,0,b.getNumSamples(),true);
 if(panic.exchange(false)){engine.reset();for(int ch=1;ch<=16;++ch)midi.addEvent(juce::MidiMessage::allNotesOff(ch),0);}
 for(auto metadata:midi){auto m=metadata.getMessage();if(!m.isController())continue;int cc=m.getControllerNumber(),target=learnTarget.exchange(-1);
 if(target>=0&&target<int(ra::specs().size())){mappings[size_t(cc)]=target;lastLearned=cc;}
 target=mappings[size_t(cc)];if(target>=0&&target<int(ra::specs().size())){pending[size_t(target)]=m.getControllerValue()/127.f;triggerAsyncUpdate();}}
 double bpm=120;if(auto* playhead=getPlayHead())if(auto position=playhead->getPosition())if(auto tempo=position->getBpm())bpm=*tempo;
 engine.process(b,midi,sample(),bpm);meter=engine.peak;activeVoices=engine.voicesActive.load();
 if(std::abs(value("stretch")-appliedStretch.load())>.001f&&!stretchBusy.load()){stretchBusy=true;revision.fetch_add(1);notify();}
}
void RaSamplerProcessor::handleAsyncUpdate(){for(size_t i=0;i<ra::specs().size();++i){float v=pending[i].exchange(-1);if(v>=0)if(auto* p=state.getParameter(ra::specs()[i].id))p->setValueNotifyingHost(v);}}
bool RaSamplerProcessor::loadSample(const juce::File& f,juce::String& error){if(!f.existsAsFile()||f.getSize()>128*1024*1024){error="File missing or larger than 128 MB.";return false;}juce::MemoryBlock bytes;if(!f.loadFileAsData(bytes)){error="Could not read audio file.";return false;}auto decoded=ra::decodeSample(bytes.getData(),bytes.getSize(),f.getFileName(),error);if(!decoded)return false;setParameter("start",0);setParameter("end",1);setParameter("loopStart",0);setParameter("loopEnd",1);setSample(decoded);setStatus("Loaded: "+f.getFileName());return true;}
void RaSamplerProcessor::setSample(std::shared_ptr<const ra::SampleData> s){std::atomic_store(&original,s);std::atomic_store(&playback,s);appliedStretch=1;panic=true;requestStretch();}
void RaSamplerProcessor::requestStretch(){stretchBusy=true;revision.fetch_add(1);notify();}
void RaSamplerProcessor::run(){int handled=0;while(!threadShouldExit()){int next=revision.load();if(next==handled){wait(100);continue;}auto src=originalSample();float factor=value("stretch");auto result=ra::stretchSample(*src,factor);if(next==revision.load()){std::atomic_store(&playback,result);appliedStretch=factor;stretchBusy=false;}handled=next;}}
void RaSamplerProcessor::reorder(int from,int to){if(from<0||from>4||to<0||to>4)return;int order[5];for(int i=0;i<5;++i)order[i]=int(value("fxOrder"+juce::String(i)));int moving=order[from];if(from<to)for(int i=from;i<to;++i)order[i]=order[i+1];else for(int i=from;i>to;--i)order[i]=order[i-1];order[to]=moving;for(int i=0;i<5;++i)setParameter("fxOrder"+juce::String(i),float(order[i]));}
void RaSamplerProcessor::resetParameters(){panic=true;for(auto& s:ra::specs())setParameter(s.id,s.initial);requestStretch();}
void RaSamplerProcessor::clearMappings(){for(auto& m:mappings)m=-1;learnTarget=-1;lastLearned=-1;}
juce::String RaSamplerProcessor::status()const{juce::ScopedLock lock(messageLock);return message;}
void RaSamplerProcessor::setStatus(const juce::String& m){juce::ScopedLock lock(messageLock);message=m;}
void RaSamplerProcessor::getStateInformation(juce::MemoryBlock& data){auto tree=state.copyState();auto s=originalSample();tree.setProperty("sampleName",s->name,nullptr);tree.setProperty("sampleData",s->encoded.toBase64Encoding(),nullptr);tree.setProperty("selectedTab",selectedTab.load(),nullptr);juce::ValueTree map("MidiMap");for(int i=0;i<128;++i){int target=mappings[size_t(i)];if(target>=0)map.setProperty("cc"+juce::String(i),ra::specs()[size_t(target)].id,nullptr);}tree.appendChild(map,nullptr);juce::MemoryOutputStream out(data,false);tree.writeToStream(out);}
void RaSamplerProcessor::setStateInformation(const void* data,int size){auto tree=juce::ValueTree::readFromData(data,size);if(!tree.isValid()||!tree.hasType("RaSampler"))return;juce::MemoryBlock bytes;juce::String error;auto s=ra::makeInitSample();auto encoded=tree.getProperty("sampleData").toString();if(encoded.isNotEmpty()){if(!bytes.fromBase64Encoding(encoded)){setStatus("Preset sample data could not be decoded.");return;}s=ra::decodeSample(bytes.getData(),bytes.getSize(),tree.getProperty("sampleName").toString(),error);if(!s){setStatus(error);return;}}
 state.replaceState(tree);selectedTab=juce::jlimit(0,5,int(tree.getProperty("selectedTab",0)));clearMappings();auto map=tree.getChildWithName("MidiMap");for(int cc=0;cc<128;++cc){auto id=map.getProperty("cc"+juce::String(cc)).toString();for(size_t i=0;i<ra::specs().size();++i)if(ra::specs()[i].id==id)mappings[size_t(cc)]=int(i);}setSample(s);setStatus("Preset restored · sample embedded");}
juce::AudioProcessorEditor* RaSamplerProcessor::createEditor(){return new RaSamplerEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new RaSamplerProcessor();}
