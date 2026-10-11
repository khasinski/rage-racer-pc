const {test}=require('node:test'),assert=require('node:assert/strict');
test('project screen shows truthful save-only status and escapes externally supplied names and values',async()=>{
  const {projectWorkspace}=await import('../renderer/project.mjs');
  const html=projectWorkspace({name:'<project>',source:{region:'PAL',fingerprint:'sha256:abc'},dirty:true,canUndo:true,canRedo:false,changes:[{kind:'car',id:'a',field:'name',value:'<script>alert(1)</script>'}]});
  assert.match(html,/&lt;project&gt;/);assert.ok(!html.includes('<script>'));
  assert.match(html,/Save project only/);assert.match(html,/does not activate/);
  assert.match(html,/data-project-history="redo" disabled/);
  assert.match(projectWorkspace(null),/Opening/);
});
