const {test}=require('node:test'),assert=require('node:assert/strict');
test('project screen shows truthful save-only status and escapes externally supplied names and values',async()=>{
  const {projectWorkspace}=await import('../renderer/project.mjs');
  const html=projectWorkspace({name:'<project>',source:{region:'PAL',fingerprint:'sha256:abc'},dirty:true,canUndo:true,canRedo:false,changes:[{kind:'car',id:'a',field:'name',value:'<script>alert(1)</script>'}]});
  assert.match(html,/&lt;project&gt;/);assert.ok(!html.includes('<script>'));
  assert.match(html,/Save project only/);assert.match(html,/does not activate/);assert.match(html,/data-action="apply-project"/);
  assert.match(html,/data-project-history="redo" disabled/);
  assert.match(projectWorkspace(null),/Opening/);
});
test('project shows selected packages and priorities, including a recoverable missing package',async()=>{
  const {projectWorkspace}=await import('../renderer/project.mjs');
  const html=projectWorkspace({name:'Project',source:{region:'PAL',fingerprint:'sha256:abc'},dirty:true,canUndo:true,canRedo:false,changes:[],selection:{packages:['a','missing'],resolutions:{'mesh:car.player.10.part.0':{winner:'a',candidates:['a','b']}}}},[{id:'a',name:'<Body replacement>'}]);
  assert.match(html,/&lt;Body replacement&gt;/);assert.ok(!html.includes('<Body replacement>'));
  assert.match(html,/Unavailable package/);assert.match(html,/select an imported replacement/);
  assert.match(html,/data-project-package="missing"/);assert.match(html,/data-action="clear-project-packages"/);
  assert.match(html,/data-project-priority="mesh:car.player.10.part.0"/);assert.match(html,/share undo\/redo/);
});
