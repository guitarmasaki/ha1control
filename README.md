#ha1control
RaspberryPiのPCM I/Fを用いたHAP-nodeJS利用の学習リモコンです。
まだ登録用のUIをちゃんと作っていないので、今のところtelnetでCUIでの制御とHomeKit経由の制御になります。

##install手順

1.最新版のRASPBIAN JESSIE LITEをdownload  
```unix
https://www.raspberrypi.org/downloads/raspbian/
```

2.8GB以上のSD-cardをMacにセットし、mountされたdevice fileを確認し、unmount状態にする  
  OS-Xの場合/Volumesにmountされているので以下のコマンドで検索出来る 
```unix
Macintosh> df | grep Volumes | grep disk  
/dev/disk3s1   30855168   4416   30850752   1%   69   482043  0%  /Volumes/UNTITLED  
```
  /dev/disk3s1のs1はパーティションなのでdiskutil umountDiskでは/dev/disk3を指定する  
```unix
Macintosh> diskutil umountDisk /dev/disk3  
```

3.RASPIBIAN JESSIE LITEのイメージの書き込み  
  &lt;zip image file&gt;は1.で取得したJESSIE LITEのイメージファイル
  &lt;SD card device file&gt; は上の例では/dev/rdisk3 (/dev/disk3のraw deviceなので/dev/rdisk3)  
```unix
Macintosh> unzip -p <zip image file> | sudo dd of=<SD card device file> bs=1m  
```
passwordを聞かれるので管理者のパスワードを入力すると数分で書き込みが完了する  

4.再度mountされてしまっているので今度はejectコマンドでmount解除する  
```unix
Macintosh> diskutil eject /dev/disk3  
```
5.出来上がったSD-cardをRaspberryPiにセットして電源を入れてloginする  
  電源をいれて30秒ほど待ったあと  
```unix
Macintosh> ssh pi@raspberrypi  
```
でloginすると  
```unix
The authenticity of host 'raspberrypi (192.168.0.59)' can't be established.  
ECDSA key fingerprint is SHA256:5AYGfBYhbs1RT8q6M/xe2EZBglqedB7MvZO9lS40VPQ.  
Are you sure you want to continue connecting (yes/no)?  
```
  と聞かれるのでyesとしてloginする  
  passwordを聞かれるのでraspberryを入力  

6.setup.shを取得して実行する  
```unix
pi@raspberrypi:~ $ curl -O https://raw.githubusercontent.com/mnakada/ha1control/master/setup.sh  
pi@raspberrypi:~ $ sudo bash setup.sh  
```
数分間installの処理が行われ、完了する  

7.piアカウントのpasswordを適当なものに変更する  
```unix
pi@raspberrypi:~ $ passwd  
Changing password for pi.  
(current) UNIX password: raspberry  
Enter new UNIX password: <new password>  
Retype new UNIX password: <new password>  
passwd: password updated successfully  
```

8.再起動  
```unix
pi@raspberrypi:~ $ sudo init 6  
```

  再起動するとhostnameがha1homeserverに変更されるので再度loginするには  
```unix
Macintosh> ssh pi@ha1homeserver  
```
で7.で登録したpasswordでloginできる。  

##使い方
  最初の状態はsony製TV向けのリモコンコードだけ登録された状態になっています。  
  リモコンコードを学習させるにはha1homeserverにloginし  
```unix
pi@ha1homeserver:~ $ telnet localhost 47900  
Trying ::1...  
Trying 127.0.0.1...  
Connected to localhost.  
Escape character is '^]'.  
```
と出るので、  
```unix
irrec hogehoge  
```
と入力するとbeep音が10秒間鳴り続けます。その間にRaspberryPiに向かって学習させたいリモコンのボタンを押してください。  
  正常に登録されればhogehogeという名前でリモコンが登録されます。  
  リモコンのコードはRaspberryPiの/var/ha1/irremote/以下に格納されます。  

##HomeKitへの登録  
  **※Appleが認証を強化すると動かなくなる可能性があります。**  
  HomeKit対応AppをAppStoreで検索して、どれか1つをinstallしてください。(Elgato Eveがおすすめ)  
  RaspberryPiと同じWiFiにiPhoneを接続した状態で起動し、アクセサリを追加を選びます。  
  名前を適当につけて次へを選択するとHA1HomeServerが見えるので選んで  
  ホームに追加->このまま追加->コードを手入力->031-45-159->次へ->完了でホームから見えるようになります。  
  そこからコマンドを選択するとリモコンが発行されます。  
  うまくいくとそのコマンドをSiriから呼び出すことが出来ます。  
  ただし、Siriは気難しいのでなかなか思い通りにコマンドを実行してくれません。会話を楽しんでみてください。  


