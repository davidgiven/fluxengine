#!/bin/sh

mkdir -p FluxEngine.app/Contents/MacOS
mkdir -p FluxEngine.app/Contents/Resources
cat >FluxEngine.app/Contents/Info.plist <<@EOF@
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleDocumentTypes</key>
	<array>
		<dict>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>*</string>
			</array>
			<key>CFBundleTypeMIMETypes</key>
			<array>
				<string>text/plain</string>
			</array>
			<key>CFBundleTypeName</key>
			<string>NSStringPboardType</string>
			<key>CFBundleTypeOSTypes</key>
			<array>
				<string>TEXT</string>
				<string>****</string>
			</array>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
		</dict>
	</array>
	<key>CFBundleExecutable</key>
	<string>FluxEngine.sh</string>
	<key>CFBundleGetInfoString</key>
	<string>FluxEngine 1.0</string>
	<key>CFBundleIconFile</key>
	<string>FluxEngine.icns</string>
	<key>CFBundleIdentifier</key>
	<string>org.fluxengine.gui</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleLocalizations</key>
	<array>
		<string>cs</string><string>de</string><string>el</string><string>en</string><string>en_gb</string><string>eo</string><string>es</string><string>fr</string><string>hu</string><string>it</string><string>nl</string><string>pl</string><string>pt</string><string>ru</string>
	</array>
	<key>CFBundleName</key>
	<string>FluxEngine</string>
        <key>CFBundleDisplayName</key>
        <string>FluxEngine</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleSignature</key>
	<string>FluxEngine</string>
	<key>CFBundleVersion</key>
	<string>1.0</string>
	<key>LSMinimumSystemVersion</key>
	<string>10.13.0</string>
	<key>LSRequiresCarbon</key>
	<true/>
	<key>NSAppleScriptEnabled</key>
	<true/>
</dict>
</plist>
@EOF@
cat >FluxEngine.app/Contents/MacOS/FluxEngine.sh <<@EOF@
#!/bin/sh

dir=\`dirname "\$0"\`
cd "\$dir"
export DYLD_FALLBACK_FRAMEWORK_PATH=../Resources
exec ./FluxEngine "\$@"
@EOF@
chmod 755 FluxEngine.app/Contents/MacOS/FluxEngine.sh
for name in `otool -L fluxengine-gui | tr -d '\t' | grep -v '^/System/' | grep -v '^/usr/lib/' | grep -v ':$' | awk -e '{print $1}'`
do
	cp "$name" FluxEngine.app/Contents/Resources
done
chmod 755 FluxEngine.app/Contents/Resources/*
cp FluxEngine.icns FluxEngine.app/Contents/Resources
chmod 644 FluxEngine.app/Contents/Resources/FluxEngine.icns
cp fluxengine-gui FluxEngine.app/Contents/MacOS/FluxEngine
chmod 755 FluxEngine.app/Contents/MacOS/FluxEngine
strip FluxEngine.app/Contents/MacOS/FluxEngine
touch FluxEngine.app
