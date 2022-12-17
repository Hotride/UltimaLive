using System;

namespace Server.Mobiles
{
    public interface UltimaLiveQuery
    {
        int QueryMobile(Mobile m, int previousBlock);
    }

    public partial class PlayerMobile : Mobile
    {
        public static UltimaLiveQuery BlockQuery;
        private int m_PreviousMapBlock = -1;
        private int m_UltimaLiveMajorVersion = 0;
        private int m_UltimaLiveMinorVersion = 0;
        private int m_UOLive_FOV_MinBlockX = -2;
        private int m_UOLive_FOV_MaxBlockX = 2;
        private int m_UOLive_FOV_MinBlockY = -2;
        private int m_UOLive_FOV_MaxBlockY = 2;
        private int m_UOLive_FOV_BlocksWidth = 5;
        private int m_UOLive_FOV_BlocksHeight = 5;

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UltimaLiveMajorVersion
        {
            get
            {
                return m_UltimaLiveMajorVersion;
            }
            set
            {
                m_UltimaLiveMajorVersion = value;
            }
        }

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UltimaLiveMinorVersion
        {
            get
            {
                return m_UltimaLiveMinorVersion;
            }
            set
            {
                m_UltimaLiveMinorVersion = value;
            }
        }

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UOLive_FOV_MinBlockX
        {
            get { return m_UOLive_FOV_MinBlockX; }
            set { m_UOLive_FOV_MinBlockX = value; }
        }

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UOLive_FOV_MaxBlockX
        {
            get { return m_UOLive_FOV_MaxBlockX; }
            set { m_UOLive_FOV_MaxBlockX = value; }
        }

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UOLive_FOV_MinBlockY
        {
            get { return m_UOLive_FOV_MinBlockY; }
            set { m_UOLive_FOV_MinBlockY = value; }
        }

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UOLive_FOV_MaxBlockY
        {
            get { return m_UOLive_FOV_MaxBlockY; }
            set { m_UOLive_FOV_MaxBlockY = value; }
        }

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UOLive_FOV_BlocksWidth
        {
            get { return m_UOLive_FOV_BlocksWidth; }
            set { m_UOLive_FOV_BlocksWidth = value; }
        }

        [CommandProperty(AccessLevel.GameMaster, true)]
        public int UOLive_FOV_BlocksHeight
        {
            get { return m_UOLive_FOV_BlocksHeight; }
            set { m_UOLive_FOV_BlocksHeight = value; }
        }
    }
}
