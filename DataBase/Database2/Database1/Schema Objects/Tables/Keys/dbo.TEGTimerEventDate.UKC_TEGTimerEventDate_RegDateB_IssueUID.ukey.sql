﻿ALTER TABLE [dbo].[TEGTimerEventDate] ADD CONSTRAINT [UKC_TEGTimerEventDate_RegDateB_IssueUID] UNIQUE CLUSTERED  ([RegDateB], [IssueUID]) WITH FILLFACTOR=80 ON [PRIMARY]

