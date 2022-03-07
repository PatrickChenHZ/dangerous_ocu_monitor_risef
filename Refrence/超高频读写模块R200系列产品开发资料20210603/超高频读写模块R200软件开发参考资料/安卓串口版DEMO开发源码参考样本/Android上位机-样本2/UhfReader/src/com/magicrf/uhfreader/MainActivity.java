package com.magicrf.uhfreader;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

import com.magicrf.uhfreaderlib.reader.Tools;
import com.magicrf.uhfreaderlib.reader.UhfReader;

public class MainActivity extends Activity implements OnClickListener ,OnItemClickListener{

	private Button buttonClear;
	private Button buttonConnect;
	private Button buttonStart;
	private TextView textVersion ;
	private ListView listViewData;
	private ArrayList<EPC> listEPC;
	private ArrayList<Map<String, Object>> listMap;
	private boolean runFlag = true;
	private boolean startFlag = false;
	private boolean connectFlag = false;
	private String serialPortPath = "/dev/ttyMT1";
	private UhfReader reader ; //����Ƶ��д�� 
	private UhfReaderDevice readerDevice; // ��д���豸��ץӴ������д����Դ
	
	private ScreenStateReceiver screenReceiver ;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setOverflowShowingAlways();
		setContentView(R.layout.main);
		initView();
		//��ȡ��д��ʵ����������Null,�򴮿ڳ�ʼ��ʧ��
		SharedPreferences sharedPortPath = getSharedPreferences("portPath", 0);
		serialPortPath =  sharedPortPath.getString("portPath", "/dev/ttyMT1");
		UhfReader.setPortPath(serialPortPath);
		reader = UhfReader.getInstance();
		//��ȡ��д���豸ʾ����������null�����豸��Դ��ʧ��
		readerDevice = UhfReaderDevice.getInstance();
		if(reader == null){
			textVersion.setText("serialport init fail");
			setButtonClickable(buttonClear, false);
			setButtonClickable(buttonStart, false);
			setButtonClickable(buttonConnect, false);
			return ;
		}
		if (readerDevice == null)
		{
			textVersion.setText("UHF reader power on failed");
//			setButtonClickable(buttonClear, false);
//			setButtonClickable(buttonStart, false);
//			setButtonClickable(buttonConnect, false);
//			return ;
		}
		try {
			Thread.sleep(100);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		//��ȡ�û����ù���,������
		SharedPreferences shared = getSharedPreferences("power", 0);
		int value = shared.getInt("value", 26);
		Log.d("", "value" + value);
		reader.setOutputPower(value);
		
		
		//��ӹ㲥��Ĭ������ʱ���ߣ�����ʱ����
		screenReceiver = new ScreenStateReceiver();
		IntentFilter filter = new IntentFilter();
		filter.addAction(Intent.ACTION_SCREEN_ON);
		filter.addAction(Intent.ACTION_SCREEN_OFF);
		registerReceiver(screenReceiver, filter);
		
		/**************************/
		
//		String serialNum = "";
//        try {
//            Class<?> classZ = Class.forName("android.os.SystemProperties");
//            Method get = classZ.getMethod("get", String.class);
//            serialNum = (String) get.invoke(classZ, "ro.serialno");
//        } catch (Exception e) {
//        }
//        Log.e("serialNum", serialNum);
		
		/*************************/
		
		
		Thread thread = new InventoryThread();
		thread.start();
		//��ʼ��������
		Util.initSoundPool(this);
	}
	
	private void initView(){
		buttonStart = (Button) findViewById(R.id.button_start);
		buttonConnect = (Button) findViewById(R.id.button_connect);
		buttonClear = (Button) findViewById(R.id.button_clear);
		listViewData = (ListView) findViewById(R.id.listView_data);
		textVersion = (TextView) findViewById(R.id.textView_version);
		buttonStart.setOnClickListener(this);
		buttonConnect.setOnClickListener(this);
		buttonClear.setOnClickListener(this);
		setButtonClickable(buttonStart, false);
		listEPC = new ArrayList<EPC>();
		listViewData.setOnItemClickListener(this);
		
	}
	
	
	@Override
	protected void onPause() {
		startFlag = false;
		super.onPause();
	}
	
	/**
	 * �̴��߳�
	 * @author Administrator
	 *
	 */
	class InventoryThread extends Thread{
		private List<byte[]> epcList;

		@Override
		public void run() {
			super.run();
			while(runFlag){
				if(startFlag){
//					reader.stopInventoryMulti()
					epcList = reader.inventoryRealTime(); //ʵʱ�̴�
					if(epcList != null && !epcList.isEmpty()){
						//������ʾ��
						Util.play(1, 0);
						for(byte[] epc:epcList){
							if (epc != null)
							{
								String epcStr = Tools.Bytes2HexString(epc, epc.length);
								addToList(listEPC, epcStr);
							}
						}
					}
					epcList = null ;
					try {
						Thread.sleep(40);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			}
		}
	}
	
	//����ȡ��EPC��ӵ�LISTVIEW
		private void addToList(final List<EPC> list, final String epc){
			runOnUiThread(new Runnable() {
				@Override
				public void run() {
					
					//��һ�ζ�������
					if(list.isEmpty()){
						EPC epcTag = new EPC();
						epcTag.setEpc(epc);
						epcTag.setCount(1);
						list.add(epcTag);
					}else{
						for(int i = 0; i < list.size(); i++){
							EPC mEPC = list.get(i);
							//list���д�EPC
							if(epc.equals(mEPC.getEpc())){
							mEPC.setCount(mEPC.getCount() + 1);
							list.set(i, mEPC);
							break;
						}else if(i == (list.size() - 1)){
							//list��û�д�epc
							EPC newEPC = new EPC();
							newEPC.setEpc(epc);
							newEPC.setCount(1);
							list.add(newEPC);
							}
						}
					}
					//��������ӵ�ListView
					listMap = new ArrayList<Map<String,Object>>();
					int idcount = 1;
					for(EPC epcdata:list){
						Map<String, Object> map = new HashMap<String, Object>();
						map.put("ID", idcount);
						map.put("EPC", epcdata.getEpc());
						map.put("COUNT", epcdata.getCount());
						idcount++;
						listMap.add(map);
					}
					listViewData.setAdapter(new SimpleAdapter(MainActivity.this,
							listMap, R.layout.listview_item, 
							new String[]{"ID", "EPC", "COUNT"}, 
							new int[]{R.id.textView_id, R.id.textView_epc, R.id.textView_count}));
				}
			});
		}

	//���ð�ť�Ƿ����
	private void setButtonClickable(Button button, boolean flag){
		button.setClickable(flag);
		if(flag){
			button.setTextColor(Color.BLACK);
		}else{
			button.setTextColor(Color.GRAY);
		}
	}
	
	@Override
	protected void onDestroy() {
		if (screenReceiver != null) {
			unregisterReceiver(screenReceiver);
		}
		runFlag = false;
		if(reader != null){
			reader.close();
		}
		if (readerDevice != null) {
			readerDevice.powerOff();
		}
		super.onDestroy();
	}
	/**
	 * ���listview
	 */
	private void clearData(){
		listEPC.removeAll(listEPC);
		listViewData.setAdapter(null);
	}
	

	@Override
	public void onClick(View v) {

		switch (v.getId()) {
		case R.id.button_start:
			if(!startFlag){
				startFlag = true ;
				buttonStart.setText(R.string.stop_inventory);
			}else{
				startFlag = false;
				buttonStart.setText(R.string.inventory);
			}
			break;
		case R.id.button_connect:
			
			byte[] versionBytes = reader.getFirmware();
			if(versionBytes != null){
//				reader.setWorkArea(3);//���ó�ŷ��
				Util.play(1, 0);
				String version = new String(versionBytes);
//				textVersion.setText(new String(versionBytes));
				setButtonClickable(buttonConnect, false);
				setButtonClickable(buttonStart, true);
			}
			setButtonClickable(buttonConnect, false);
			setButtonClickable(buttonStart, true);
			break;
			
		case R.id.button_clear:
			clearData();
			break;
		default:
			break;
		}
	}
	 
	private int value = 2600;
//	private int values = 432 ;
//	private int mixer = 0;
//	private int if_g = 0;
	
	@Override
	public void onItemClick(AdapterView<?> adapter, View view, int position, long id) {
		TextView epcTextview = (TextView) view.findViewById(R.id.textView_epc);
		String epc = epcTextview.getText().toString();
		//ѡ��EPC
//		reader.selectEPC(Tools.HexString2Bytes(epc));
		
		Toast.makeText(getApplicationContext(), epc, 0).show();
		Intent intent = new Intent(this, MoreHandleActivity.class);
		intent.putExtra("epc", epc);
		startActivity(intent);
	}
	
	@Override
	public boolean onMenuItemSelected(int featureId, MenuItem item) {
//		Log.e("", "adfasdfasdf");
//		Intent intent = new Intent(this, SettingActivity.class);
//		startActivity(intent);
		Intent intent = new Intent(this, SettingPower.class);
		startActivity(intent);
		return super.onMenuItemSelected(featureId, item);
	}
	
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		
		return true;
	}
	
	@Override
  	public boolean onMenuOpened(int featureId, Menu menu) {
		if (featureId == Window.FEATURE_ACTION_BAR && menu != null) {
			if (menu.getClass().getSimpleName().equals("MenuBuilder")) {
				try {
					Method m = menu.getClass().getDeclaredMethod(
							"setOptionalIconsVisible", Boolean.TYPE);
					m.setAccessible(true);
					m.invoke(menu, true);
				} catch (Exception e) {
				}
			}
		}
	return super.onMenuOpened(featureId, menu);
	}
	
	/**
	 * ��actionbar����ʾ�˵���ť
	 */
	private void setOverflowShowingAlways() {
		try {
			ViewConfiguration config = ViewConfiguration.get(this);
			Field menuKeyField = ViewConfiguration.class
					.getDeclaredField("sHasPermanentMenuKey");
			menuKeyField.setAccessible(true);
			menuKeyField.setBoolean(config, false);
			
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
