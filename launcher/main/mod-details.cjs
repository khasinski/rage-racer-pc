const limits={name:200,author:120,version:80,description:1200};
function validateDetails(value){
 if(!value||typeof value!=='object'||Array.isArray(value))throw Error('Invalid mod details');
 for(const key of Object.keys(value))if(!Object.hasOwn(limits,key))throw Error('Unknown mod detail: '+key);
 if(typeof value.name!=='string'||!value.name.trim())throw Error('Enter a mod name');
 for(const [key,text] of Object.entries(value)){
  const invalid=key==='description'?/[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]/:/[\x00-\x1f\x7f]/;
  if(typeof text!=='string'||text.length>limits[key]||invalid.test(text))throw Error('Invalid mod detail: '+key);
 }
 return {...value};
}
module.exports={validateDetails};
