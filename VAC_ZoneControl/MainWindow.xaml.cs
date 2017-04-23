using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Runtime.InteropServices;
using System.Reflection;
//using Midi;
using NAudio;
using NAudio.Mixer;
using NAudio.CoreAudioApi;

namespace VAC_ZoneControl
{
    
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //bool minimizedToTray;
        //NotifyIcon notifyIcon;

        List<SourceControl> sourceList;
        List<ZoneControl> zoneList;
        Boolean updateMapping;
        //OutputDevice midiDeviceCard1, midiDeviceCard2;

        public MainWindow()
        {
            this.Closed += new EventHandler(MainWindow_Closed);
            updateMapping = true;
            InitializeComponent();

            //InitializeMidiDevices();
            InitializeSources();
            InitializeZones();
            

            disableZones();
            
        }

        void MainWindow_Closed(object sender, EventArgs e)
        {
            // Close any opened repeaters
            foreach (ZoneControl zoneControl in zoneList)
            {
                zoneControl.stopRepeater();

                switch (zoneControl.zoneType)
                {
                    case ZoneType.Den :
                        Properties.Settings.Default.VolumeDen = zoneControl.getVolume();
                        break;
                    case ZoneType.Kitchen:
                        Properties.Settings.Default.VolumeKitchen = zoneControl.getVolume();
                        break;
                    case ZoneType.Dining:
                        Properties.Settings.Default.VolumeDining = zoneControl.getVolume();
                        break;
                    case ZoneType.Living:
                        Properties.Settings.Default.VolumeLiving = zoneControl.getVolume();
                        break;
                    case ZoneType.OutsideHouse:
                        Properties.Settings.Default.VolumeOutsideHouse = zoneControl.getVolume();
                        break;
                    case ZoneType.OutsideFence:
                        Properties.Settings.Default.VolumeOutsideFence = zoneControl.getVolume();
                        break;

                }
            }
            
            Properties.Settings.Default.Save();
        }
        

        private void InitializeSources()
        {
            sourceList = new List<SourceControl>();

            SourceControl sourceControl;

            // Default
            sourceControl = new SourceControl();
            sourceControl.btnSource = btnSourceDefault;
            sourceControl.sourceType = SourceType.Default;
            sourceControl.device = Properties.Settings.Default.DeviceSourceDefault;
            sourceList.Add(sourceControl);

            // iPod
            sourceControl = new SourceControl();
            sourceControl.btnSource = btnSourceIPod;
            sourceControl.sourceType = SourceType.iPod;
            sourceControl.device = Properties.Settings.Default.DeviceSourceIPod;
            sourceList.Add(sourceControl);

            // TurnTable
            sourceControl = new SourceControl();
            sourceControl.btnSource = btnSourceTurntable;
            sourceControl.sourceType = SourceType.Turntable;
            sourceControl.device = Properties.Settings.Default.DeviceSourceTurntable;
            sourceList.Add(sourceControl);

            // Line-in
            sourceControl = new SourceControl();
            sourceControl.btnSource = btnSourceLineIn;
            sourceControl.sourceType = SourceType.Linein;
            sourceControl.device = Properties.Settings.Default.DeviceSourceLineIn;
            sourceList.Add(sourceControl);

        }

        private void InitializeZones()
        {
            zoneList = new List<ZoneControl>();
            ZoneControl zoneControl;

            // Den
            zoneControl = new ZoneControl();
            zoneControl.btnZone = btnZoneDen;
            zoneControl.sliderVolume = volDen;
            zoneControl.zoneType = ZoneType.Den;
            zoneControl.repeaterPath = Properties.Settings.Default.RepeaterDen;
            zoneControl.repeaterDevice = Properties.Settings.Default.RepeaterDeviceZoneDen;
            zoneControl.playbackDevice = Properties.Settings.Default.PlaybackDeviceZoneDen;
            //zoneControl.midiDevice = midiDeviceCard1;
            //zoneControl.midiChannel = (Channel)Properties.Settings.Default.ChannelDen;
            zoneControl.setVolume(Properties.Settings.Default.VolumeDen);
            zoneList.Add(zoneControl);

            // Kitchen
            zoneControl = new ZoneControl();
            zoneControl.btnZone = btnZoneKitchen;
            zoneControl.sliderVolume = volKitchen;
            zoneControl.zoneType = ZoneType.Kitchen;
            zoneControl.repeaterPath = Properties.Settings.Default.RepeaterKitchen;
            zoneControl.repeaterDevice = Properties.Settings.Default.RepeaterDeviceZoneKitchen;
            zoneControl.playbackDevice = Properties.Settings.Default.PlaybackDeviceZoneKitchen;
            //zoneControl.midiDevice = midiDeviceCard1;
            //zoneControl.midiChannel = (Channel)Properties.Settings.Default.ChannelKitchen;
            zoneControl.setVolume(Properties.Settings.Default.VolumeKitchen);
            zoneList.Add(zoneControl);

            // Dining
            zoneControl = new ZoneControl();
            zoneControl.btnZone = btnZoneDining;
            zoneControl.sliderVolume = volDining;
            zoneControl.zoneType = ZoneType.Dining;
            zoneControl.repeaterPath = Properties.Settings.Default.RepeaterDining;
            zoneControl.repeaterDevice = Properties.Settings.Default.RepeaterDeviceZoneDining;
            zoneControl.playbackDevice = Properties.Settings.Default.PlaybackDeviceZoneDining;
            //zoneControl.midiDevice = midiDeviceCard1;
            //zoneControl.midiChannel = (Channel)Properties.Settings.Default.ChannelDining;
            zoneControl.setVolume(Properties.Settings.Default.VolumeDining);
            zoneList.Add(zoneControl);

            // Living
            zoneControl = new ZoneControl();
            zoneControl.btnZone = btnZoneLiving;
            zoneControl.sliderVolume = volLiving;
            zoneControl.zoneType = ZoneType.Living;
            zoneControl.repeaterPath = Properties.Settings.Default.RepeaterLiving;
            zoneControl.repeaterDevice = Properties.Settings.Default.RepeaterDeviceZoneLiving;
            zoneControl.playbackDevice = Properties.Settings.Default.PlaybackDeviceZoneLiving;
            //zoneControl.midiDevice = midiDeviceCard2;
            //zoneControl.midiChannel = (Channel)Properties.Settings.Default.ChannelLiving;
            zoneControl.setVolume(Properties.Settings.Default.VolumeLiving);
            zoneList.Add(zoneControl);

            // Outside House
            zoneControl = new ZoneControl();
            zoneControl.btnZone = btnZoneOutsideHouse;
            zoneControl.sliderVolume = volOutsideHouse;
            zoneControl.zoneType = ZoneType.OutsideHouse;
            zoneControl.repeaterPath = Properties.Settings.Default.RepeaterOutsideHouse;
            zoneControl.repeaterDevice = Properties.Settings.Default.RepeaterDeviceZoneOutsideHouse;
            zoneControl.playbackDevice = Properties.Settings.Default.PlaybackDeviceZoneOutsideHouse;
            //zoneControl.midiDevice = midiDeviceCard2;
            //zoneControl.midiChannel = (Channel)Properties.Settings.Default.ChannelOutsideHouse;
            zoneControl.setVolume(Properties.Settings.Default.VolumeOutsideHouse);
            zoneList.Add(zoneControl);

            // Outside Fence
            zoneControl = new ZoneControl();
            zoneControl.btnZone = btnZoneOutsideFence;
            zoneControl.sliderVolume = volOutsideFence;
            zoneControl.zoneType = ZoneType.OutsideFence;
            zoneControl.repeaterPath = Properties.Settings.Default.RepeaterOutsideFence;
            zoneControl.repeaterDevice = Properties.Settings.Default.RepeaterDeviceZoneOutsideFence;
            zoneControl.playbackDevice = Properties.Settings.Default.PlaybackDeviceZoneOutsideFence;
            //zoneControl.midiDevice = midiDeviceCard2;
            //zoneControl.midiChannel = (Channel)Properties.Settings.Default.ChannelOutsideFence;
            zoneControl.setVolume(Properties.Settings.Default.VolumeOutsideFence);
            zoneList.Add(zoneControl);

            // Television
            zoneControl = new ZoneControl();
            zoneControl.btnZone = btnZoneTelevision;
            zoneControl.zoneType = ZoneType.Television;
            zoneControl.repeaterPath = Properties.Settings.Default.RepeaterTelevision;
            zoneControl.repeaterDevice = Properties.Settings.Default.DeviceZoneTelevision;
            zoneControl.playbackDevice = Properties.Settings.Default.PlaybackDeviceZoneTelevision;
            zoneList.Add(zoneControl);

        }

        /*
        private void InitializeMidiDevices()
        {
            if (OutputDevice.InstalledDevices.Count > 0)
            {
                foreach (OutputDevice device in OutputDevice.InstalledDevices)
                {
                    if (String.Compare(device.Name, Properties.Settings.Default.kxDevice1) == 0)
                    {
                        midiDeviceCard1 = device;
                    }
                    else if (String.Compare(device.Name, Properties.Settings.Default.kxDevice2) == 0)
                    {
                        midiDeviceCard2 = device;
                    }
                }
            }
            
            
        }
        */

        private SourceControl findSourceControlChecked()
        {
            SourceControl ret = new SourceControl();
            ret.sourceType = SourceType.None;

            foreach (SourceControl sourceControl in sourceList)
            {
                if (sourceControl.btnSource.IsChecked == true)
                {
                    return sourceControl;
                }
            }

            return ret;
        }

        private SourceControl findSourceControlType(SourceType _sourceType)
        {
            SourceControl ret = new SourceControl();
            ret.sourceType = SourceType.None;

            foreach ( SourceControl sourceControl in sourceList )
            {
                if (sourceControl.sourceType == _sourceType)
                {
                    return sourceControl;
                }
            }

            return ret;
        }

        private SourceControl findSourceControlName(ToggleButton _toggleButton)
        {
            SourceControl ret = new SourceControl();
            ret.sourceType = SourceType.None;

            foreach (SourceControl sourceControl in sourceList)
            {
                if (sourceControl.btnSource.Name == _toggleButton.Name)
                {
                    return sourceControl;
                }
            }

            return ret;
        }

        private ZoneControl findZoneControlType(ZoneType _zoneType)
        {
            ZoneControl ret = new ZoneControl();
            ret.zoneType = ZoneType.None;

            foreach (ZoneControl zoneControl in zoneList)
            {
                if (zoneControl.zoneType == _zoneType)
                {
                    return zoneControl;
                }
            }

            return ret;
        }

        private ZoneControl findZoneControlName(ToggleButton _toggleButton)
        {
            ZoneControl ret = new ZoneControl();
            ret.zoneType = ZoneType.None;

            foreach (ZoneControl zoneControl in zoneList)
            {
                if (zoneControl.btnZone.Name == _toggleButton.Name)
                {
                    return zoneControl;
                }
            }

            return ret;
        }

        private ZoneControl findZoneBySlider(Slider _sliderVol)
        {
            ZoneControl ret = new ZoneControl();
            ret.zoneType = ZoneType.None;

            foreach (ZoneControl zoneControl in zoneList)
            {
                if (zoneControl.sliderVolume.Name == _sliderVol.Name)
                {
                    return zoneControl;
                }
            }

            return ret;
        }

        private void disableZones()
        {
            updateMapping = false;
            foreach (ZoneControl zoneControl in zoneList)
            {
                zoneControl.btnZone.IsEnabled = false;

                if (zoneControl.selectedSource != SourceType.None)
                {
                    zoneControl.btnZone.IsChecked = true;
                }
                else
                {
                    zoneControl.btnZone.IsChecked = false;
                }
            }
            updateMapping = true;
        }

        private void enableZones(SourceControl _sourceControl)
        {
            updateMapping = false;
            foreach (ZoneControl zoneControl in zoneList)
            {
                zoneControl.btnZone.IsEnabled = true;
                if (_sourceControl.isZoneSelected(zoneControl.zoneType) )
                {
                    zoneControl.btnZone.IsChecked = true;
                }
                else
                {
                    zoneControl.btnZone.IsChecked = false;
                }
            }
            updateMapping = true;
        }

        private void btnClose_Click(object sender, RoutedEventArgs e)
        {
            WindowState = System.Windows.WindowState.Minimized;
        }

        private void btnSource_Checked(object sender, RoutedEventArgs e)
        {
            ToggleButton toggleButton = (ToggleButton)sender;
            if (toggleButton != null)
            {
                SourceControl sourceControl = this.findSourceControlName(toggleButton);
                if (sourceControl != null )
                {
                    // Uncheck other sources
                    foreach( SourceControl sourceControlOther in sourceList )
                    {
                        if (sourceControlOther.sourceType != sourceControl.sourceType)
                        {
                            sourceControlOther.btnSource.IsChecked = false;
                        }
                    }

                    // Enable zones
                    if (sourceControl.sourceType != SourceType.None)
                    {
                        enableZones(sourceControl);
                    }
                }

            }
        }

        private void btnSource_Unchecked(object sender, RoutedEventArgs e)
        {
            ToggleButton toggleButton = (ToggleButton)sender;
            if (toggleButton != null)
            {
                SourceControl sourceControl = this.findSourceControlName(toggleButton);
                if (sourceControl != null)
                {
                    if (sourceControl.sourceType != SourceType.None)
                    {
                        disableZones();
                    }
                }

            }
        }

        private void btnZone_Checked(object sender, RoutedEventArgs e)
        {
            if (updateMapping)
            {
                ToggleButton toggleButton = (ToggleButton)sender;
                if (toggleButton != null)
                {
                    ZoneControl zoneControl = this.findZoneControlName(toggleButton);
                    if (zoneControl != null)
                    {
                        // Get checked Source
                        SourceControl sourceControlChecked = this.findSourceControlChecked();
                        if (sourceControlChecked.sourceType != SourceType.None)
                        {
                            // Remove existing selected source
                            if (zoneControl.selectedSource != SourceType.None)
                            {
                                SourceControl sourceControlPrevious = findSourceControlType(zoneControl.selectedSource);
                                if (sourceControlPrevious.sourceType != SourceType.None)
                                {
                                    sourceControlPrevious.selectedZones.Remove(zoneControl.zoneType);
                                }
                            }

                            // Set selected source
                            zoneControl.selectedSource = sourceControlChecked.sourceType;

                            // Add to selected source
                            sourceControlChecked.selectZone(zoneControl.zoneType);

                            // Stop current repeater
                            zoneControl.stopRepeater();

                            // Start new repeater
                            zoneControl.startRepeater(sourceControlChecked);

                        }
                    }
                }
            }
            
        }

        private void btnZone_Unchecked(object sender, RoutedEventArgs e)
        {
            if (updateMapping)
            {
                ToggleButton toggleButton = (ToggleButton)sender;
                if (toggleButton != null)
                {
                    ZoneControl zoneControl = this.findZoneControlName(toggleButton);
                    if (zoneControl != null)
                    {
                        // Remove from selected zone
                        SourceControl previousSourceSelected = findSourceControlType(zoneControl.selectedSource);
                        if (previousSourceSelected.sourceType != SourceType.None)
                        {
                            previousSourceSelected.selectedZones.Remove(zoneControl.zoneType);
                        }

                        zoneControl.selectedSource = SourceType.None;

                        // Stop current repeater
                        zoneControl.stopRepeater();
                    }
                }
            }
        }

        private void sliderVol_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            int volume = Convert.ToInt32(e.NewValue);
            ZoneControl zoneControl = findZoneBySlider((Slider)sender);
            if (zoneControl.zoneType != ZoneType.None)
            {
                zoneControl.setVolume(volume, false);
            }
        }

        
    }

    public enum SourceType { None, Default, iPod, Turntable, Linein }

    public enum ZoneType { None, Den, Kitchen, Dining, Living, OutsideHouse, OutsideFence, Television }


    public class SourceControl
    {
        public SourceType sourceType;
        public ToggleButton btnSource;
        public String device;
        List<ZoneType> _selectedZones;

        public SourceControl()
        {
            _selectedZones = new List<ZoneType>();
        }

        public void selectZone(ZoneType _zoneType)
        {
            _selectedZones.Add(_zoneType);
        }

        public void unselectZone(ZoneType _zoneType)
        {
            _selectedZones.Remove(_zoneType);
        }

        public List<ZoneType> selectedZones
        {
            get
            {
                return _selectedZones;
            }
        }

        public Boolean isZoneSelected(ZoneType _zoneType)
        {
            foreach (ZoneType selectedZone in _selectedZones)
            {
                if (selectedZone == _zoneType)
                {
                    return true;
                }
            }
            return false;
        }

    }

    public class ZoneControl
    {
        public ZoneType zoneType;
        public ToggleButton btnZone;
        public Slider sliderVolume;
        public SourceType selectedSource;
        public String repeaterPath;
        public String repeaterDevice;
        public String playbackDevice;
        //public OutputDevice midiDevice;
        //public Channel midiChannel;
        public MMDevice mmDevice;
        private int volume;

        private Process repeaterProcess;

        public void stopRepeater()
        {
            if (repeaterProcess != null)
            {
                if (repeaterProcess.HasExited == false)
                {
                    repeaterProcess.Kill();
                }
            }
        }

        public void startRepeater(SourceControl sourceControl)
        {
            String arguments = String.Format("/input:\"{0}\" /output:\"{1}\" /bitspersample:{2} /channels:{3} /bufferms:{4} /buffers:{5} /autostart", 
                                                sourceControl.device,
                                                this.repeaterDevice,
                                                Properties.Settings.Default.RepeaterBitsPerSample.ToString(),
                                                Properties.Settings.Default.RepeaterChannels.ToString(),
                                                Properties.Settings.Default.RepeaterBufferMS.ToString(),
                                                Properties.Settings.Default.RepeaterBuffers.ToString()                                            
                                                );

            ProcessStartInfo startInfo = new ProcessStartInfo(repeaterPath,arguments );
            startInfo.WindowStyle = ProcessWindowStyle.Minimized;

            repeaterProcess = Process.Start(startInfo);
        }

        public void setVolume(int newVolume, bool updateSlider = true)
        {
            if (this.mmDevice == null)
            {
                MMDeviceEnumerator enumerator = new MMDeviceEnumerator();
                MMDeviceCollection collection = enumerator.EnumerateAudioEndPoints(DataFlow.Render, DeviceState.Active);
                System.Diagnostics.Trace.TraceInformation("Count {0}", collection.Count);

                foreach (MMDevice device in collection)
                {
                    if (device.FriendlyName == this.playbackDevice)
                    {
                        this.mmDevice = device;
                        break;
                    }
                }
            }

            if (this.mmDevice != null)
            {
                volume = newVolume;
                float convertedVol = Convert.ToSingle(Convert.ToDouble(volume) / 100.0);
                this.mmDevice.AudioEndpointVolume.MasterVolumeLevelScalar = convertedVol;
            }

            //volume = newVolume;
            //int convertedVolume;
            //if (midiDevice != null)
            //{
            //    if (!midiDevice.IsOpen)
            //    {
            //        midiDevice.Open();
            //    }
            //    convertedVolume = volume * 16383 / 100;
            //    midiDevice.SendPitchBend(midiChannel, convertedVolume);
            //    midiDevice.Close();
            //}



            if (updateSlider && sliderVolume != null)
            {
                sliderVolume.Value = volume;
            }
        }

        public int getVolume()
        {
            return volume;
        }

    }

    class RoundingConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value != null)
            {
                double dblValue = (double)value;
                return (int)dblValue;
            }
            return 0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
