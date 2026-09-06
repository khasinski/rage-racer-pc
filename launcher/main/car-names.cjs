// Keep authored suffixes (wheel/grade) and stable model keys; only the display
// name follows the disc. Tables come from the compiled save library.
module.exports=function carName(authored,region,names){
 const index=names.international.findIndex(name=>authored===name||authored.startsWith(name+' '));
 if(index<0)return authored;
 const selected=region==='NTSC-J'?names.japanese:names.international;
 return selected[index]+authored.slice(names.international[index].length);
};
