// The older package screen shares the project builder; it never replaces the
// whole game mod root or removes previously applied builds.
module.exports=async function publishMods(service){
  service.requireReady();
  if(service.game)throw Error('Close the game before saving its mods');
  if(!service.state.mods.some(mod=>mod.enabled))throw Error('Enable at least one mod before saving for the game');
  const project=await service.project();
  await service.applyProject(project.revision);
  return (await require('./project-build.cjs').activeBuild(service)).directory;
};
