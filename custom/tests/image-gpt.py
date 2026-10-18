
import pathlib,tempfile,subprocess,os,struct,zlib,json
root=pathlib.Path(__file__).resolve().parents[2]
r=pathlib.Path(tempfile.mkdtemp(prefix='gpt-build-probe-',dir='/home/homelede'))
src=subprocess.check_output(['git','show','HEAD:scripts/gen_image_generic.sh'],cwd=root,text=True)
fixed=(root/'scripts/gen_image_generic.sh').read_text()
(r/'original.sh').write_text(src);(r/'fixed.sh').write_text(fixed)
(r/'boot').mkdir();(r/'boot/kernel').write_bytes(b'isolated GPT test\n');(r/'rootfs').write_bytes(b'rootfs-test'+bytes(4096))
env=os.environ|{'PATH':str(root/'staging_dir/host/bin')+':'+os.environ['PATH'],'PADDING':'1','GUID':'620dac02-b08e-2bb9-3ab9-7d3f2be33100','SIGNATURE':'620dac02'}
def check(p):
 with p.open('rb') as f:
  m=f.read(512);h=f.read(512);f.seek(0,2);size=f.tell();n=size//512
  def header(b):
   if b[:8]!=b'EFI PART':return None
   hs=struct.unpack_from('<I',b,12)[0];tmp=bytearray(b[:hs]);crc=struct.unpack_from('<I',tmp,16)[0];tmp[16:20]=bytes(4)
   assert zlib.crc32(tmp)==crc
   return {'self':struct.unpack_from('<Q',b,24)[0],'alt':struct.unpack_from('<Q',b,32)[0],'lba':struct.unpack_from('<Q',b,72)[0],'count':struct.unpack_from('<I',b,80)[0],'esize':struct.unpack_from('<I',b,84)[0],'crc':struct.unpack_from('<I',b,88)[0]}
  a=header(h);assert a
  f.seek(a['lba']*512);t=f.read(a['count']*a['esize']);assert zlib.crc32(t)==a['crc']
  f.seek((n-1)*512);b=header(f.read(512));ok=False
  if b:
   f.seek(b['lba']*512);bt=f.read(b['count']*b['esize']);ok=(bt==t and zlib.crc32(bt)==b['crc'] and a['alt']==n-1 and b['alt']==1)
  return {'sectors':n,'primary_alt':a['alt'],'backup_valid_at_end':ok,'pmbr_length':struct.unpack_from('<I',m,458)[0]},t
results={};tables={}
for variant in ['original','fixed']:
 image=r/(variant+'.img')
 with (r/(variant+'.log')).open('w') as log:
  subprocess.run(['sh',str(r/(variant+'.sh')),str(image),'32',str(r/'boot'),'2016',str(r/'rootfs'),'256'],env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 results[variant],tables[variant]=check(image)
assert not results['original']['backup_valid_at_end'];assert results['fixed']['backup_valid_at_end'];assert tables['original']==tables['fixed'];assert results['fixed']['pmbr_length']==results['fixed']['sectors']-1
for fmt in ['vmdk','qcow2','vdi']:
 subprocess.run(['qemu-img','convert','-f','raw','-O',fmt,str(r/'fixed.img'),str(r/('fixed.'+fmt))],check=True)
 subprocess.run(['qemu-img','convert','-f',fmt,'-O','raw',str(r/('fixed.'+fmt)),str(r/('roundtrip-'+fmt+'.img'))],check=True)
 results[fmt],t=check(r/('roundtrip-'+fmt+'.img'));assert results[fmt]['backup_valid_at_end'] and t==tables['fixed']
print(json.dumps(results,indent=2));print('PARTITION_ENTRIES_IDENTICAL');print('EXPERIMENT_DIR',r)
(r/'results.json').write_text(json.dumps(results,indent=2))
subprocess.run(['diff','-u',str(r/'original.sh'),str(r/'fixed.sh')])
