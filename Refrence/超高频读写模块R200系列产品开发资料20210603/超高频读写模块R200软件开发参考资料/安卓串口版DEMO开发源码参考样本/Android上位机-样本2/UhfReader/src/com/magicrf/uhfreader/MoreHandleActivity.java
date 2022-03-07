package com.magicrf.uhfreader;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.magicrf.uhfreaderlib.reader.Tools;
import com.magicrf.uhfreaderlib.reader.UhfReader;

public class MoreHandleActivity extends Activity implements OnClickListener{
	private TextView textViewEPC;
	private Spinner spinnerMemBank;//数据区
	private EditText editPassword;//密码
	private EditText editAddr;//起始地址
	private EditText editLength;//读取的数据长度
	private Button	buttonRead;
	private Button buttonWrite;
	private EditText editWriteData;//要写入的数据
	private EditText editReadData;//读取数据展示区
	private Button buttonClear;
	private final String[] strMemBank = {"RESERVE", "EPC", "TID", "USER"};//RESERVE EPC TID USER分别对应0,1,2,3
	private ArrayAdapter<String> adapterMemBank;
	private Spinner spinnerLockType;//要锁定的类型
	private Spinner spinnerLockMemSpace; //要锁定的数据区
	private Button buttonLock;//锁定按钮
	private EditText editKillPassword;//销毁密码
	private Button buttonKill;//销毁按钮
	private ArrayAdapter<CharSequence> adapterLockType;
	private final String[] strLockMemSpace = {"Kill Pwd", "Access Pwd", "EPC", "TID", "USER"};//分别对应0,1,2,3,4
	private ArrayAdapter<String> adapterLockMemSpace;
	private int membank;//数据区
	private int lockMemSpace; 
	private int addr = 0;//起始地址
	private int length = 1;//读取数据的长度
	private int lockType;//
	private Button buttonBack ;
	
	private UhfReader reader;
	private UhfReaderDevice readerDevice;
	String epc = "";
	
	private String TAG = "MorehandleActivity";//DEBUG
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.iso1800_6c);
		epc = getIntent().getStringExtra("epc");
		initView();
		listener();
		textViewEPC.setText(epc);
		//获取读写器实例，若返回Null,则串口初始化失败
		reader = UhfReader.getInstance();
		//获取读写器设备示例，若返回null，则设备电源打开失败
		readerDevice = UhfReaderDevice.getInstance();
		if(reader == null){
			Toast.makeText(getApplicationContext(), "serialport init fail", 0).show();
			return ;
		}
		if(readerDevice == null){
			Toast.makeText(getApplicationContext(), "UHF reader power on failed", 0).show();
			return ;
		}
		
		reader.selectEpc(Tools.HexString2Bytes(epc));
	}
	
	private void initView(){
		textViewEPC = (TextView) findViewById(R.id.textViewEPC);
		this.spinnerMemBank = (Spinner) findViewById(R.id.spinner_membank);
		this.editAddr = (EditText) findViewById(R.id.edittext_addr);
		this.editLength = (EditText) findViewById(R.id.edittext_length);
		this.editPassword = (EditText) findViewById(R.id.editTextPassword);
		this.buttonRead = (Button) findViewById(R.id.button_read);
		this.buttonWrite = (Button) findViewById(R.id.button_write);
		this.buttonClear = (Button) findViewById(R.id.button_readClear);
		this.buttonLock = (Button) findViewById(R.id.button_lock_6c);
		this.buttonKill = (Button) findViewById(R.id.button_kill_6c);
		this.buttonBack = (Button) findViewById(R.id.button_back);
		this.editKillPassword = (EditText) findViewById(R.id.edit_kill_password);
		this.editWriteData = (EditText) findViewById(R.id.edittext_write);
		this.editReadData = (EditText) findViewById(R.id.linearLayout_readData);
		this.adapterMemBank = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, strMemBank);
		this.adapterMemBank.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.spinnerMemBank.setAdapter(adapterMemBank);
		this.spinnerLockMemSpace = (Spinner) findViewById(R.id.spinner_lock_memspace);
		this.adapterLockMemSpace = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, strLockMemSpace);
		this.adapterLockMemSpace.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.spinnerLockMemSpace.setAdapter(adapterLockMemSpace);
		this.spinnerLockType = (Spinner) findViewById(R.id.spinner_lock_type);
		this.adapterLockType =  ArrayAdapter.createFromResource(this, R.array.arr_lockType, android.R.layout.simple_spinner_item);
		this.adapterLockType.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		this.spinnerLockType.setAdapter(adapterLockType);
		
		setButtonClickable(buttonKill, true);
		setButtonClickable(buttonLock, true);
	}

	//监听
	private void listener(){
		this.buttonClear.setOnClickListener(this);
		this.buttonRead.setOnClickListener(this);
		this.buttonWrite.setOnClickListener(this);
		this.buttonKill.setOnClickListener(this);
		this.buttonLock.setOnClickListener(this);
		this.buttonBack.setOnClickListener(this);
		spinnerMemBank.setOnItemSelectedListener(new OnItemSelectedListener() {

			@Override
			public void onItemSelected(AdapterView<?> arg0, View arg1,
					int arg2, long arg3) {
				membank = arg2;
				Log.i(TAG, "memeBank " + membank);
				
			}

			@Override
			public void onNothingSelected(AdapterView<?> arg0) {
				// TODO Auto-generated method stub
				
			}
		});
		spinnerLockMemSpace.setOnItemSelectedListener(new OnItemSelectedListener() {

			@Override
			public void onItemSelected(AdapterView<?> arg0, View arg1,
					int arg2, long arg3) {
				lockMemSpace = arg2;
				Log.i(TAG,"lock MemSpace " + lockMemSpace);
			}

			@Override
			public void onNothingSelected(AdapterView<?> arg0) {
				// TODO Auto-generated method stub
				
			}
			
		});
		spinnerLockType.setOnItemSelectedListener(new OnItemSelectedListener() {

			@Override
			public void onItemSelected(AdapterView<?> arg0, View arg1,
					int arg2, long arg3) {
				lockType = arg2 ;
				Log.i(TAG,"lockType " + lockType);
			}

			@Override
			public void onNothingSelected(AdapterView<?> arg0) {
				// TODO Auto-generated method stub
				
			}
		});
		
	}
	
	
	//设置按钮是否可用
	private void setButtonClickable(Button button, boolean flag){
		button.setClickable(flag);
		if(flag){
			button.setTextColor(Color.BLACK);
		}else{
			button.setTextColor(Color.GRAY);
		}
	}
	
	
	@Override
	public void onClick(View v) {
		byte[] accessPassword = Tools.HexString2Bytes(editPassword.getText().toString());
		byte[] killPassword = Tools.HexString2Bytes(editKillPassword.getText().toString());
		addr = Integer.valueOf(editAddr.getText().toString());
		length = Integer.valueOf(editLength.getText().toString());
		switch (v.getId()) {
		//读标签数据
		case R.id.button_read:
			if(accessPassword.length != 4){
				Toast.makeText(getApplicationContext(), "密码为4个字节", Toast.LENGTH_SHORT).show();
				return;
			}
			//读取数据区数据
			byte[] data = reader.readFrom6C(membank, addr, length, accessPassword);
			if(data != null && data.length > 1){
				String dataStr =  Tools.Bytes2HexString(data, data.length);
//				Toast.makeText(getApplicationContext(),dataStr, 0).show();
				editReadData.append("读数据：" + dataStr + "\n");
			}else{
//				Toast.makeText(getApplicationContext(), "读数据失败", Toast.LENGTH_SHORT).show();
				if(data != null){
					editReadData.append("读数据失败，错误码：" + (data[0]&0xff) + "\n");
					return;
				}
				editReadData.append("读数据失败，返回为空"  + "\n");
			}
			break;
		//写标签数据
		case R.id.button_write:
			if(accessPassword.length != 4){
				Toast.makeText(getApplicationContext(), "密码为4个字节", Toast.LENGTH_SHORT).show();
				return;
			}
			String writeData = editWriteData.getText().toString();
			if(writeData.length()%4 != 0){
				Toast.makeText(getApplicationContext(), "写入数据的长度以字为单位，1word = 2bytes", Toast.LENGTH_SHORT).show();
			}
			byte[] dataBytes = Tools.HexString2Bytes(writeData);
			//dataLen = dataBytes/2 dataLen是以字为单位的
			boolean writeFlag = reader.writeTo6C(accessPassword, membank, addr, dataBytes.length/2, dataBytes);
			if(writeFlag){
				editReadData.append("写数据成功！"  + "\n");
			}else{
				editReadData.append("写数据失败！"  + "\n");
			}
			break;
		//锁定标签
		case R.id.button_lock_6c:
			if(accessPassword.length != 4){
				Toast.makeText(getApplicationContext(), "密码为4个字节", Toast.LENGTH_SHORT).show();
				return;
			}
			boolean lockFlag = reader.lock6C(accessPassword, lockMemSpace, lockType);
			if(lockFlag){
				editReadData.append("锁定成功！"  + "\n");
			}else{
				editReadData.append("锁定失败！"  + "\n");
			}
			
			break;
		//销毁标签
		case R.id.button_kill_6c:
			if(killPassword.length != 4){
				Toast.makeText(getApplicationContext(), "灭活密码为4个字节", Toast.LENGTH_SHORT).show();
				return;
			}
			boolean isKillPasswdAllZero = true;
			for (byte b : killPassword) {
				if (b != 0) {
					isKillPasswdAllZero = false;
				}
			}
			if (isKillPasswdAllZero == true) {
				Toast.makeText(getApplicationContext(), "灭活密码全为0时会灭活失败", Toast.LENGTH_SHORT).show();
			}
			boolean killFlag = reader.kill6C(killPassword);
			if(killFlag){
				editReadData.append("灭活成功！"  + "\n");
			}else{
				editReadData.append("灭活失败！"  + "\n");
			}
			break;
		//清空内容
		case R.id.button_readClear:
			editReadData.setText("");
			break;
		//返回
		case R.id.button_back:
			finish();
			break;
		default:
			break;
		}
		
	}

}
